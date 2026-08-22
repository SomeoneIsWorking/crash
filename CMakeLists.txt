cmake_minimum_required(VERSION 3.24)
project(crash_port LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Crash C++ must be compiled with clang++; got ${CMAKE_CXX_COMPILER_ID} "
                      "(${CMAKE_CXX_COMPILER}). Reconfigure with CC=clang CXX=clang++.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
enable_testing()

# PSXPORT_DIR — the framework. external/psxport is NOT a git submodule (2026-08-16): it is a symlink
# to the workspace's shared framework clone, or a private clone at this repo's psxport.pin on a fresh
# machine; tools/psxport_sync.py --auto establishes whichever applies. Defaults to external/psxport so
# a bare clone of this repo builds standalone.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "`python3 tools/psxport_sync.py --auto`, or point PSXPORT_DIR at a psxport checkout.")
endif()
# RECORD WHAT THIS CONFIGURE RESOLVED, so `tools/psxport_sync.py --check` can compare the built
# framework against the one psxport.pin RECORDS.
execute_process(COMMAND git -C "${PSXPORT_DIR}" rev-parse HEAD
                OUTPUT_VARIABLE PSXPORT_RESOLVED_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _psxport_rev_rc)
if(NOT _psxport_rev_rc EQUAL 0)
  set(PSXPORT_RESOLVED_SHA "unknown")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/psxport_resolved.txt"
     "dir = ${PSXPORT_DIR}\ncommit = ${PSXPORT_RESOLVED_SHA}\n")
set(PSXPORT_BUILD_SMOKE ON CACHE BOOL "Build the framework agnosticism smoke" FORCE)
add_subdirectory("${PSXPORT_DIR}" psxport_build)
add_custom_target(crash_scaffold DEPENDS psxport_smoke)

add_library(crash_boundary_runtime STATIC "${CMAKE_CURRENT_SOURCE_DIR}/game/core/boundary_runtime.cpp")
target_include_directories(crash_boundary_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game/core")
target_compile_features(crash_boundary_runtime PUBLIC cxx_std_20)
target_link_libraries(crash_boundary_runtime PUBLIC psxport)

add_library(crash1_runtime STATIC "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_runtime.cpp")
target_include_directories(crash1_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core")
target_compile_features(crash1_runtime PUBLIC cxx_std_20)
target_link_libraries(crash1_runtime PUBLIC crash_boundary_runtime)

add_executable(crash1_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_runtime.cpp")
target_include_directories(crash1_runtime_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_features(crash1_runtime_test PRIVATE cxx_std_20)
target_link_libraries(crash1_runtime_test PRIVATE crash1_runtime)
add_test(NAME crash1_runtime_inheritance COMMAND crash1_runtime_test)

function(add_crt0_boundary_title title)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/executable.json" title_manifest_json)
  string(JSON title_executable GET "${title_manifest_json}" executable)
  set(title_cache "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/${title}")
  set(title_generated "${CMAKE_CURRENT_SOURCE_DIR}/generated/${title}")
  set(title_exe "${title_cache}/${title_executable}")

  add_library(${title}_runtime STATIC
              "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/core/${title}_runtime.cpp")
  target_include_directories(${title}_runtime
                             PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/core")
  target_compile_features(${title}_runtime PUBLIC cxx_std_20)
  target_link_libraries(${title}_runtime PUBLIC crash_boundary_runtime)
  set_target_properties(
    ${title}_runtime
    PROPERTIES CRASH_TITLE_CACHE "${title_cache}" CRASH_TITLE_GENERATED "${title_generated}"
  )

  add_executable(${title}_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/${title}_runtime.cpp")
  target_include_directories(${title}_runtime_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_features(${title}_runtime_test PRIVATE cxx_std_20)
  target_link_libraries(${title}_runtime_test PRIVATE ${title}_runtime)
  add_test(NAME ${title}_runtime_inheritance COMMAND ${title}_runtime_test)

  add_executable(${title}_runtime_facts
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/${title}_runtime_facts.cpp")
  target_compile_features(${title}_runtime_facts PRIVATE cxx_std_20)
  target_link_libraries(${title}_runtime_facts PRIVATE ${title}_runtime)

  add_custom_target(
    ${title}_oracle_boot_check
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/provision_title.py"
      --title ${title} --discdump "$<TARGET_FILE:discdump>"
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_executable.py"
      --title ${title} --selftest --exe "${title_exe}"
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_runtime_image.py"
      --title ${title} --exe "${title_exe}" --extractor "$<TARGET_FILE:crt0_extract>"
      --runtime-facts "$<TARGET_FILE:${title}_runtime_facts>" --selftest
    COMMAND "$<TARGET_FILE:oracle_spike>"
    COMMAND
      "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/oracle/crossvalidate_crt0.py"
      "${title_exe}" --build "${CMAKE_BINARY_DIR}/psxport_build" --steps 400000
    DEPENDS ${title}_runtime_facts crt0_extract discdump oracle_spike oracle_trace
    USES_TERMINAL
    VERBATIM
  )
endfunction()

add_crt0_boundary_title(crash2)
add_crt0_boundary_title(crash3)

add_test(
  NAME crash_cpp_policy
  COMMAND
    "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_cpp_style.py"
    --root "${CMAKE_CURRENT_SOURCE_DIR}" --compile-commands "${CMAKE_BINARY_DIR}"
)
add_test(
  NAME crash_title_provision_tests
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_provision_titles.py"
)
set_tests_properties(crash_title_provision_tests PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/executable.json" CRASH1_MANIFEST_JSON)
string(JSON CRASH1_EXECUTABLE GET "${CRASH1_MANIFEST_JSON}" executable)
set(CRASH1_EXE "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/crash1/${CRASH1_EXECUTABLE}")
set(CRASH1_GENERATED "${CMAKE_CURRENT_SOURCE_DIR}/generated/crash1")
add_custom_target(
  crash1_recomp_generate
  COMMAND
    "${CMAKE_COMMAND}" -E env "PSXPORT_DIR=${PSXPORT_DIR}" "PYTHONDONTWRITEBYTECODE=1"
    "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/crash1_recomp.py"
    --emit --exe "${CRASH1_EXE}" --output "${CRASH1_GENERATED}"
  BYPRODUCTS
    "${CRASH1_GENERATED}/overlay_table.c"
    "${CRASH1_GENERATED}/shard_0.c"
    "${CRASH1_GENERATED}/shard_disp.c"
    "${CRASH1_GENERATED}/rec_decls.h"
    "${CRASH1_GENERATED}/rec_sources.cmake"
    "${CRASH1_GENERATED}/.recomp_version"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/crash1_recomp.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_executable.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/executable.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/recomp_seeds.json"
    "${PSXPORT_DIR}/tools/recomp/emit.py"
    "${PSXPORT_DIR}/tools/recomp/decode.py"
    "${PSXPORT_DIR}/tools/recomp/psexe.py"
  COMMENT "Crash 1: emitting the resident substrate from the verified USA executable"
  VERBATIM
)
add_executable(
  crash1_recomp_boundary EXCLUDE_FROM_ALL
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_recomp_boundary.cpp"
  "${CRASH1_GENERATED}/overlay_table.c"
  "${CRASH1_GENERATED}/shard_0.c"
  "${CRASH1_GENERATED}/shard_disp.c"
)
add_dependencies(crash1_recomp_boundary crash1_recomp_generate)
set_source_files_properties(
  "${CRASH1_GENERATED}/overlay_table.c"
  "${CRASH1_GENERATED}/shard_0.c"
  "${CRASH1_GENERATED}/shard_disp.c"
  PROPERTIES LANGUAGE CXX
)
target_include_directories(crash1_recomp_boundary PRIVATE "${CRASH1_GENERATED}")
target_compile_features(crash1_recomp_boundary PRIVATE cxx_std_20)
target_compile_options(
  crash1_recomp_boundary PRIVATE -O1 -foptimize-sibling-calls -fno-strict-aliasing -fwrapv
)
target_link_libraries(crash1_recomp_boundary PRIVATE crash1_runtime)
add_custom_target(
  crash1_recomp_boundary_check
  COMMAND
    "${CMAKE_COMMAND}" -E env "PSXPORT_DIR=${PSXPORT_DIR}" "PYTHONDONTWRITEBYTECODE=1"
    "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/crash1_recomp.py"
    --selftest --exe "${CRASH1_EXE}" --build "${CMAKE_BINARY_DIR}"
    --runner "$<TARGET_FILE:crash1_recomp_boundary>"
  DEPENDS crash1_recomp_boundary crt0_extract oracle_trace
  USES_TERMINAL
  VERBATIM
)
add_custom_target(
  crash1_oracle_boot_check
  COMMAND
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/provision_title.py"
    --title crash1 --discdump "$<TARGET_FILE:discdump>"
  COMMAND "$<TARGET_FILE:oracle_spike>"
  COMMAND
    "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/oracle/crossvalidate_crt0.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/crash1/${CRASH1_EXECUTABLE}"
    --build "${CMAKE_BINARY_DIR}/psxport_build" --steps 400000
  DEPENDS crt0_extract discdump oracle_spike oracle_trace
  USES_TERMINAL
  VERBATIM
)
