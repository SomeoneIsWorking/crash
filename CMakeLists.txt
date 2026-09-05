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
option(PSXPORT_BUILD_SMOKE "Build the framework agnosticism smoke" ON)
add_subdirectory("${PSXPORT_DIR}" psxport_build)
if(PSXPORT_BUILD_SMOKE)
  add_custom_target(crash_scaffold DEPENDS psxport_smoke)
endif()

function(configure_title_vsync target title)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/executable.json" title_vsync_manifest)
  string(JSON title_vsync_entry GET "${title_vsync_manifest}" runtime vsync entry)
  string(JSON title_vsync_end GET "${title_vsync_manifest}" runtime vsync end)
  target_compile_definitions(
    ${target}
    PRIVATE
      "CRASH_TITLE_VSYNC_ENTRY=${title_vsync_entry}u"
      "CRASH_TITLE_VSYNC_END=${title_vsync_end}u"
  )
endfunction()

function(configure_crash1_native_contract target)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/executable.json" crash1_contract_manifest)
  foreach(fact
          "CRASH1_STATIC_CONSTRUCTORS_ENTRY;runtime;native_boot;static_constructors;entry"
          "CRASH1_USE_CD_ADDRESS;runtime;native_boot;use_cd_address"
          "CRASH1_INIT_ENTRY;runtime;native_boot;init;entry"
          "CRASH1_CD_INITIALIZE_ENTRY;runtime;native_boot;cd_initialize;entry"
          "CRASH1_CD_INITIALIZE_END;runtime;native_boot;cd_initialize;end"
          "CRASH1_DISC_INDEX_ENTRY;runtime;native_boot;disc_index;entry"
          "CRASH1_DISC_INDEX_END;runtime;native_boot;disc_index;end"
          "CRASH1_CD_CONTROL_ENTRY;runtime;native_boot;cd_control;entry"
          "CRASH1_CD_CONTROL_END;runtime;native_boot;cd_control;end"
          "CRASH1_CD_CONTROL_F_ENTRY;runtime;native_boot;cd_control_f;entry"
          "CRASH1_CD_CONTROL_F_END;runtime;native_boot;cd_control_f;end"
          "CRASH1_CD_SYNC_WRAPPER_ENTRY;runtime;native_boot;cd_sync_wrapper;entry"
          "CRASH1_CD_SYNC_WRAPPER_END;runtime;native_boot;cd_sync_wrapper;end"
          "CRASH1_CD_SYNC_ENTRY;runtime;native_boot;cd_sync;entry"
          "CRASH1_CD_SYNC_END;runtime;native_boot;cd_sync;end"
          "CRASH1_CD_READ_ENTRY;runtime;native_boot;cd_read;entry"
          "CRASH1_CD_READ_END;runtime;native_boot;cd_read;end"
          "CRASH1_CD_READ_SYNC_ENTRY;runtime;native_boot;cd_read_sync;entry"
          "CRASH1_CD_READ_SYNC_END;runtime;native_boot;cd_read_sync;end"
          "CRASH1_CALLBACK_INITIALIZE_ENTRY;runtime;native_boot;callback_initialize;entry"
          "CRASH1_CALLBACK_INITIALIZE_END;runtime;native_boot;callback_initialize;end"
          "CRASH1_GPU_WATCHDOG_START_ENTRY;runtime;native_boot;gpu_watchdog_start;entry"
          "CRASH1_GPU_WATCHDOG_START_END;runtime;native_boot;gpu_watchdog_start;end"
          "CRASH1_GPU_WATCHDOG_CHECK_ENTRY;runtime;native_boot;gpu_watchdog_check;entry"
          "CRASH1_GPU_WATCHDOG_CHECK_END;runtime;native_boot;gpu_watchdog_check;end"
          "CRASH1_CORE_LOOP_ENTRY;runtime;native_frame;core_loop;entry"
          "CRASH1_CORE_LOOP_END;runtime;native_frame;core_loop;end"
          "CRASH1_FRAME_ITERATION_ENTRY;runtime;native_frame;iteration;entry"
          "CRASH1_FRAME_ITERATION_END;runtime;native_frame;iteration;end"
          "CRASH1_FRAME_TRANSITION_ENTRY;runtime;native_frame;transition;entry"
          "CRASH1_FRAME_TRANSITION_END;runtime;native_frame;transition;end"
          "CRASH1_BIOS_PAD_READ_WORD_ADDRESS;runtime;native_frame;bios_pad_read;address"
          "CRASH1_GPU_UPDATE_ENTRY;runtime;native_frame;gpu_update;entry"
          "CRASH1_GPU_UPDATE_END;runtime;native_frame;gpu_update;end"
          "CRASH1_FIRST_VSYNC;runtime;native_frame;first_vsync"
          "CRASH1_AFTER_FIRST_VSYNC;runtime;native_frame;after_first_vsync"
          "CRASH1_SECOND_VSYNC;runtime;native_frame;second_vsync"
          "CRASH1_AFTER_VSYNC;runtime;native_frame;after_vsync"
          "CRASH1_DONE_ADDRESS;runtime;native_frame;done;address"
          "CRASH1_TICKS_ELAPSED_ADDRESS;runtime;native_frame;ticks_elapsed_address"
          "CRASH1_DISPLAY_CONTEXT_ADDRESS;runtime;native_frame;display_context_address"
          "CRASH1_ROOT_COUNTER_INCREMENT;runtime;native_frame;root_counter_increment"
          "CRASH1_SET_ROOT_COUNTER;runtime;native_frame;set_root_counter"
          "CRASH1_START_ROOT_COUNTER;runtime;native_frame;start_root_counter"
          "CRASH1_STOP_ROOT_COUNTER;runtime;native_frame;stop_root_counter")
    list(POP_FRONT fact definition)
    string(JSON value GET "${crash1_contract_manifest}" ${fact})
    target_compile_definitions(${target} PRIVATE "${definition}=${value}u")
  endforeach()
endfunction()

add_library(
  crash_boundary_runtime STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/game/core/boundary_runtime.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/game/core/dynarec_dispatch.cpp"
)
target_include_directories(crash_boundary_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game/core")
target_compile_features(crash_boundary_runtime PUBLIC cxx_std_20)
target_link_libraries(crash_boundary_runtime PUBLIC psxport)

add_library(
  crash_native_frame_contract STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/game/core/native_frame_loop_contract.cpp"
)
target_include_directories(crash_native_frame_contract PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game/core")
target_compile_features(crash_native_frame_contract PUBLIC cxx_std_20)
target_link_libraries(crash_native_frame_contract PUBLIC psxport)

add_library(
  crash1_runtime STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_runtime.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_bios_pad_input.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_callback_boot.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_cd_boot.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_disc_index_io.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_frame_driver.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_gpu_watchdog.cpp"
)
target_include_directories(crash1_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core")
target_compile_features(crash1_runtime PUBLIC cxx_std_20)
target_link_libraries(crash1_runtime PUBLIC crash_boundary_runtime crash_native_frame_contract)
configure_title_vsync(crash1_runtime crash1)
configure_crash1_native_contract(crash1_runtime)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/executable.json" crash1_manifest_json)
string(JSON crash1_first_syscall_entry GET "${crash1_manifest_json}" runtime first_syscall_entry)
string(JSON crash1_first_bios_dispatch_address
       GET "${crash1_manifest_json}" runtime first_bios_dispatch_address)
string(JSON crash1_first_bios_dispatch_function
       GET "${crash1_manifest_json}" runtime first_bios_dispatch_function)
string(JSON crash1_first_bios_dispatch_return_address
       GET "${crash1_manifest_json}" runtime first_bios_dispatch_return_address)
string(JSON crash1_post_get_c0_dispatch_address
       GET "${crash1_manifest_json}" runtime post_get_c0_dispatch_address)
string(JSON crash1_post_get_c0_dispatch_function
       GET "${crash1_manifest_json}" runtime post_get_c0_dispatch_function)
string(JSON crash1_post_get_c0_dispatch_return_address
       GET "${crash1_manifest_json}" runtime post_get_c0_dispatch_return_address)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/bios_contract.json" crash1_bios_contract_json)
string(JSON crash1_get_c0_table_function
       GET "${crash1_bios_contract_json}" get_c0_table function)
string(JSON crash1_c0_exception_handler_slot
       GET "${crash1_bios_contract_json}" exception_handler table_slot)
string(JSON crash1_c0_exception_handler_address
       GET "${crash1_bios_contract_json}" exception_handler retail_address)

add_library(
  crash1_boot_frontier STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_boot_frontier.cpp"
)
target_compile_definitions(
  crash1_boot_frontier
  PRIVATE
    "CRASH1_FIRST_SYSCALL_ENTRY=${crash1_first_syscall_entry}u"
    "CRASH1_FIRST_BIOS_DISPATCH_ADDRESS=${crash1_first_bios_dispatch_address}u"
    "CRASH1_FIRST_BIOS_DISPATCH_FUNCTION=${crash1_first_bios_dispatch_function}u"
    "CRASH1_FIRST_BIOS_DISPATCH_RETURN_ADDRESS=${crash1_first_bios_dispatch_return_address}u"
    "CRASH1_POST_GET_C0_DISPATCH_ADDRESS=${crash1_post_get_c0_dispatch_address}u"
    "CRASH1_POST_GET_C0_DISPATCH_FUNCTION=${crash1_post_get_c0_dispatch_function}u"
    "CRASH1_POST_GET_C0_DISPATCH_RETURN_ADDRESS=${crash1_post_get_c0_dispatch_return_address}u"
)
target_compile_features(crash1_boot_frontier PUBLIC cxx_std_20)
target_link_libraries(crash1_boot_frontier PUBLIC crash1_runtime)

if(BUILD_TESTING)
  add_executable(crash_dynarec_dispatch_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/dynarec_dispatch.cpp")
  target_link_libraries(crash_dynarec_dispatch_test PRIVATE crash1_runtime)
  add_test(NAME crash_dynarec_dispatch COMMAND crash_dynarec_dispatch_test)

  add_executable(crash1_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_runtime.cpp")
  target_include_directories(crash1_runtime_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_features(crash1_runtime_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_runtime_test PRIVATE crash1_runtime)
  add_test(NAME crash1_runtime_inheritance COMMAND crash1_runtime_test)

  add_executable(crash1_cd_boot_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_cd_boot.cpp")
  target_compile_features(crash1_cd_boot_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_cd_boot_test PRIVATE crash1_runtime)
  add_test(NAME crash1_cd_boot COMMAND crash1_cd_boot_test)

  add_executable(crash1_bios_pad_input_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_bios_pad_input.cpp")
  target_compile_features(crash1_bios_pad_input_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_bios_pad_input_test PRIVATE crash1_runtime)
  add_test(NAME crash1_bios_pad_input COMMAND crash1_bios_pad_input_test)

  add_executable(crash1_disc_index_io_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_disc_index_io.cpp")
  target_include_directories(crash1_disc_index_io_test PRIVATE "${PSXPORT_DIR}/tests")
  target_compile_features(crash1_disc_index_io_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_disc_index_io_test PRIVATE crash1_runtime)
  add_test(NAME crash1_disc_index_io COMMAND crash1_disc_index_io_test)

  add_executable(crash1_callback_boot_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_callback_boot.cpp")
  target_compile_features(crash1_callback_boot_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_callback_boot_test PRIVATE crash1_runtime)
  add_test(NAME crash1_callback_boot COMMAND crash1_callback_boot_test)

  add_executable(crash1_gpu_watchdog_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_gpu_watchdog.cpp")
  target_compile_features(crash1_gpu_watchdog_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_gpu_watchdog_test PRIVATE crash1_runtime)
  add_test(NAME crash1_gpu_watchdog COMMAND crash1_gpu_watchdog_test)

  add_executable(crash1_boot_frontier_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_boot_frontier.cpp")
  target_compile_features(crash1_boot_frontier_test PRIVATE cxx_std_20)
  target_link_libraries(crash1_boot_frontier_test PRIVATE crash1_boot_frontier)
  add_test(NAME crash1_boot_frontier COMMAND crash1_boot_frontier_test)

  add_executable(crash1_c0_exception_contract_probe
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_c0_exception_contract.cpp")
  target_compile_definitions(
    crash1_c0_exception_contract_probe
    PRIVATE
      "CRASH1_GET_C0_TABLE_FUNCTION=${crash1_get_c0_table_function}u"
      "CRASH1_C0_EXCEPTION_HANDLER_SLOT=${crash1_c0_exception_handler_slot}u"
      "CRASH1_C0_EXCEPTION_HANDLER_ADDRESS=${crash1_c0_exception_handler_address}u"
  )
  target_compile_features(crash1_c0_exception_contract_probe PRIVATE cxx_std_20)
  target_link_libraries(crash1_c0_exception_contract_probe PRIVATE crash1_runtime)
  add_test(NAME crash1_c0_exception_contract COMMAND crash1_c0_exception_contract_probe)

  add_executable(crash1_runtime_facts "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash1_runtime_facts.cpp")
  target_include_directories(crash1_runtime_facts PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_features(crash1_runtime_facts PRIVATE cxx_std_20)
  target_link_libraries(crash1_runtime_facts PRIVATE crash1_runtime)
endif()

function(add_crt0_boundary_title title)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/executable.json" title_manifest_json)
  string(JSON title_executable GET "${title_manifest_json}" executable)
  set(title_cache "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/${title}")
  set(title_exe "${title_cache}/${title_executable}")

  add_library(
    ${title}_runtime STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/core/${title}_runtime.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/core/${title}_frame_driver.cpp"
  )
  target_include_directories(${title}_runtime
                             PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/titles/${title}/core")
  target_compile_features(${title}_runtime PUBLIC cxx_std_20)
  target_link_libraries(${title}_runtime PUBLIC crash_boundary_runtime crash_native_frame_contract)
  configure_title_vsync(${title}_runtime ${title})
  set_target_properties(${title}_runtime PROPERTIES CRASH_TITLE_CACHE "${title_cache}")

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
  add_executable(crash_native_frame_contract_test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/native_frame_loop_contract.cpp")
  target_compile_features(crash_native_frame_contract_test PRIVATE cxx_std_20)
  target_link_libraries(crash_native_frame_contract_test
                        PRIVATE crash1_runtime crash2_runtime crash3_runtime)
  add_test(NAME crash_native_frame_contract COMMAND crash_native_frame_contract_test)

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
  add_test(
    NAME crash_dependency_provenance
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_psxport_sync.py"
  )
endif()
set(crash1_exe "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/crash1/SCUS_949.00")
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

add_executable(
  crash1_port
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core/crash1_port.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/main.cpp"
)
target_include_directories(
  crash1_port
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/game/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/titles/crash1/core"
)
target_compile_features(crash1_port PRIVATE cxx_std_20)
target_link_libraries(crash1_port PRIVATE crash1_runtime)
set_target_properties(crash1_port PROPERTIES ENABLE_EXPORTS ON)
if(BUILD_TESTING)
  add_test(NAME crash1_product_help COMMAND $<TARGET_FILE:crash1_port> --help)
  set_tests_properties(crash1_product_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")
  add_test(NAME crash1_product_help_short COMMAND $<TARGET_FILE:crash1_port> -h)
  set_tests_properties(crash1_product_help_short PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")
  add_test(
    NAME crash1_execution_boundary
    COMMAND
      "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_execution_boundary.py"
      --root "${CMAKE_CURRENT_SOURCE_DIR}"
      --cmake "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
      --binary "$<TARGET_FILE:crash1_port>"
  )
endif()
