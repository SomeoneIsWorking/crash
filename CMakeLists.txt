cmake_minimum_required(VERSION 3.24)
project(crash_port LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

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

if(BUILD_TESTING)
  add_executable(crash1_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_runtime.cpp")
  target_include_directories(crash1_runtime_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_features(crash1_runtime_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_runtime_test PRIVATE crash1_runtime)
  add_test(NAME crash1_runtime_inheritance COMMAND crash1_runtime_test)

  add_executable(crash1_runtime_facts "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_runtime_facts.cpp")
  target_include_directories(crash1_runtime_facts PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_features(crash1_runtime_facts PRIVATE cxx_std_20)
  target_link_libraries(crash1_runtime_facts PRIVATE crash1_runtime)
endif()

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

  if(BUILD_TESTING)
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
  endif()
endfunction()

add_crt0_boundary_title(crash2)
add_crt0_boundary_title(crash3)

if(BUILD_TESTING)
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
  add_test(
    NAME crash_launcher_tests
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_run.py"
  )
  set_tests_properties(crash_launcher_tests PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
endif()
function(add_resident_recomp_title title tool)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/executable.json" title_manifest_json)
  string(JSON title_executable GET "${title_manifest_json}" executable)
  set(title_exe "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/${title}/${title_executable}")
  set(title_generated "${CMAKE_CURRENT_SOURCE_DIR}/generated/${title}")

  add_custom_target(
    ${title}_recomp_generate
    COMMAND
      "${CMAKE_COMMAND}" -E env "PSXPORT_DIR=${PSXPORT_DIR}" "PYTHONDONTWRITEBYTECODE=1"
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}"
      --emit --exe "${title_exe}" --output "${title_generated}"
    BYPRODUCTS
      "${title_generated}/overlay_table.c"
      "${title_generated}/shard_0.c"
      "${title_generated}/shard_disp.c"
      "${title_generated}/rec_decls.h"
      "${title_generated}/rec_sources.cmake"
      "${title_generated}/.recomp_version"
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/resident_recomp.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_executable.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/executable.json"
      "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/recomp_seeds.json"
      "${PSXPORT_DIR}/tools/recomp/emit.py"
      "${PSXPORT_DIR}/tools/recomp/decode.py"
      "${PSXPORT_DIR}/tools/recomp/psexe.py"
    COMMENT "${title}: emitting the resident substrate from its verified USA executable"
    VERBATIM
  )
  add_executable(
    ${title}_recomp_boundary EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/game/core/enter_critical_frontier.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game/core/resident_program.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/${title}_recomp_boundary.cpp"
    "${title_generated}/overlay_table.c"
    "${title_generated}/shard_0.c"
    "${title_generated}/shard_disp.c"
  )
  add_dependencies(${title}_recomp_boundary ${title}_recomp_generate)
  set_source_files_properties(
    "${title_generated}/overlay_table.c"
    "${title_generated}/shard_0.c"
    "${title_generated}/shard_disp.c"
    PROPERTIES LANGUAGE CXX
  )
  target_include_directories(${title}_recomp_boundary PRIVATE "${title_generated}")
  target_compile_features(${title}_recomp_boundary PRIVATE cxx_std_20)
  target_compile_options(
    ${title}_recomp_boundary PRIVATE -O1 -foptimize-sibling-calls -fno-strict-aliasing -fwrapv
  )
  target_link_libraries(${title}_recomp_boundary PRIVATE ${title}_runtime)
  add_custom_target(
    ${title}_recomp_boundary_check
    COMMAND
      "${CMAKE_COMMAND}" -E env "PSXPORT_DIR=${PSXPORT_DIR}" "PYTHONDONTWRITEBYTECODE=1"
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}"
      --selftest --exe "${title_exe}" --build "${CMAKE_BINARY_DIR}"
      --runner "$<TARGET_FILE:${title}_recomp_boundary>"
    DEPENDS ${title}_recomp_boundary crt0_extract oracle_trace
    USES_TERMINAL
    VERBATIM
  )
  set(${title}_exe "${title_exe}" PARENT_SCOPE)
endfunction()

add_resident_recomp_title(crash1 crash1_recomp.py)
add_resident_recomp_title(crash2 scus_94154_recomp.py)
add_resident_recomp_title(crash3 scus_94244_recomp.py)
if(BUILD_TESTING)
  add_custom_target(
    crash1_oracle_boot_check
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/provision_title.py"
      --title crash1 --discdump "$<TARGET_FILE:discdump>"
    COMMAND "$<TARGET_FILE:oracle_spike>"
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_runtime_image.py"
      --title crash1 --exe "${crash1_exe}" --extractor "$<TARGET_FILE:crt0_extract>"
      --runtime-facts "$<TARGET_FILE:crash1_runtime_facts>" --selftest
    COMMAND
      "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/oracle/crossvalidate_crt0.py"
      "${crash1_exe}"
      --build "${CMAKE_BINARY_DIR}/psxport_build" --steps 400000
    DEPENDS crash1_runtime_facts crt0_extract discdump oracle_spike oracle_trace
    USES_TERMINAL
    VERBATIM
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/crash1/rec_sources.cmake")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/executable.json" crash1_manifest_json)
  string(JSON crash1_first_syscall_entry GET "${crash1_manifest_json}" runtime first_syscall_entry)
  set(crash1_generated "${CMAKE_CURRENT_SOURCE_DIR}/generated/crash1")
  include("${crash1_generated}/rec_sources.cmake")
  set(crash1_generated_sources)
  foreach(generated_source IN LISTS GEN_REC_SRCS)
    list(APPEND crash1_generated_sources "${crash1_generated}/${generated_source}")
  endforeach()
  add_executable(
    crash1_port
    "${CMAKE_CURRENT_SOURCE_DIR}/game/core/enter_critical_frontier.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game/core/resident_program.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_port.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/main.cpp"
    ${crash1_generated_sources}
  )
  add_dependencies(crash1_port crash1_recomp_generate)
  set_source_files_properties(${crash1_generated_sources} PROPERTIES LANGUAGE CXX)
  target_include_directories(
    crash1_port
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/game/core"
      "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core"
      "${crash1_generated}"
  )
  target_compile_definitions(
    crash1_port PRIVATE "CRASH1_FIRST_SYSCALL_ENTRY=${crash1_first_syscall_entry}u"
  )
  target_compile_features(crash1_port PRIVATE cxx_std_20)
  target_compile_options(
    crash1_port PRIVATE -O1 -foptimize-sibling-calls -fno-strict-aliasing -fwrapv
  )
  target_link_libraries(crash1_port PRIVATE crash1_runtime)
  set_target_properties(
    crash1_port
    PROPERTIES
      ENABLE_EXPORTS ON
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin"
  )
else()
  message(
    STATUS
      "crash1_port: NOT configured because generated/crash1/rec_sources.cmake is absent; "
      "provision SCUS_949.00 and run tools/crash1_recomp.py --emit before reconfiguring"
  )
endif()
