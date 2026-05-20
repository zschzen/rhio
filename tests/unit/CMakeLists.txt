# ============================================================================
# rhio - unit test configuration
# ============================================================================
# Configure from the repository root:
#   cmake -S . -B build-tests -DRHIO_BUILD_TESTS=ON
#
# Useful targets:
#   ctest --test-dir build-tests
#   cmake --build build-tests --target rhio_unit_report
#   cmake --build build-tests --target rhio_unit_report_<suite>
# ============================================================================

cmake_minimum_required(VERSION 3.26..4.3 FATAL_ERROR)

project(rhio_unit_tests LANGUAGES C CXX)

# --------------------------------------------------------------------
# Paths
# --------------------------------------------------------------------
set(RHIO_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

# --------------------------------------------------------------------
# CTest
# --------------------------------------------------------------------
enable_testing()

# --------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------
# Reuse CPM when this project is included by the root build
if(NOT COMMAND CPMAddPackage)
  include("${RHIO_ROOT_DIR}/cmake/CPM.cmake")

  if(EXISTS "${RHIO_ROOT_DIR}/package-lock.cmake")
    CPMUsePackageLock("${RHIO_ROOT_DIR}/package-lock.cmake")
  endif()
endif()

# Include rktest
CPMAddPackage(
  NAME rktest
  GITHUB_REPOSITORY Warwolt/rktest
  GIT_TAG 6d8f9354bfff2bfd0944ffd29264ed6058dcaa5b
  OPTIONS
    "rktest_build_tests OFF"
    "rktest_build_samples OFF"
)

# --------------------------------------------------------------------
# Shared Test Configuration
# --------------------------------------------------------------------
add_library(rhio_unit_config INTERFACE)
target_include_directories(rhio_unit_config INTERFACE "${RHIO_ROOT_DIR}")
target_compile_definitions(rhio_unit_config INTERFACE RHIO_LOG_SUPPORT)

# --------------------------------------------------------------------
# C Unit Test Sources
# --------------------------------------------------------------------
# These files are also scanned below to keep CTest registration automatic
set(RHIO_UNIT_TEST_SOURCES
  command_queue_tests.c
  device_tests.c
  logging_tests.c
  status_tests.c
)

# --------------------------------------------------------------------
# C Unit Test Runner
# --------------------------------------------------------------------
add_executable(
  rhio_unit_tests
  # Provides the single RHIO_IMPLEMENTATION translation unit required by rhio.h
  unit_impl.c
  ${RHIO_UNIT_TEST_SOURCES}
)
target_compile_features(rhio_unit_tests PRIVATE c_std_99)
target_link_libraries(rhio_unit_tests PRIVATE rhio_unit_config rktest)

# --------------------------------------------------------------------
# rktest Arguments
# --------------------------------------------------------------------
# Report targets keep the native rktest layout and colors for local use
set(RHIO_RKTEST_REPORT_ARGS
  "--rktest_color=yes"
)

# CTest runs one rktest case per CTest entry. Hiding filenames keeps the
# failure output focused on the rhio test name selected by the CTest filter
set(RHIO_RKTEST_CTEST_ARGS
  ${RHIO_RKTEST_REPORT_ARGS}
  "--rktest_print_filenames=0"
)

# --------------------------------------------------------------------
# CTest Registration Helpers
# --------------------------------------------------------------------
# Registers one CTest test that runs exactly one native rktest case
function(rhio_add_rktest_case suite test_case)
  add_test(
    NAME "rhio.unit.${suite}.${test_case}"
    COMMAND rhio_unit_tests ${RHIO_RKTEST_CTEST_ARGS} "--rktest_filter=${suite}.${test_case}"
  )

  set_tests_properties(
    "rhio.unit.${suite}.${test_case}"
    PROPERTIES
      LABELS "rhio;unit;${suite}"
  )
endfunction()

# Discover TEST(suite, case) declarations from the source list so CTest stays
# in sync when a test is renamed, added, or removed
function(rhio_collect_rktest_cases out_var)
  # Collect discovered cases
  set(rktest_cases)

  # Match TEST declarations
  # ----------------------------------------------------------
  # Match TEST(suite, case) declarations
  set(test_pattern "TEST[ \t\r\n]*\\([ \t\r\n]*[A-Za-z_][A-Za-z0-9_]*[ \t\r\n]*,[ \t\r\n]*[A-Za-z_][A-Za-z0-9_]*[ \t\r\n]*\\)")
  # Capture the suite and case names from a match already known to be valid
  set(test_capture_pattern "TEST[ \t\r\n]*\\([ \t\r\n]*([A-Za-z_][A-Za-z0-9_]*)[ \t\r\n]*,[ \t\r\n]*([A-Za-z_][A-Za-z0-9_]*)[ \t\r\n]*\\)")

  # Scan source files
  # ----------------------------------------------------------
  foreach(source_file ${ARGN})
    set(source_path "${CMAKE_CURRENT_LIST_DIR}/${source_file}")
    # Re-run configure when a source file changes its TEST(...) declarations
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_path}")
    file(READ "${source_path}" source_text)
    string(REGEX MATCHALL "${test_pattern}" source_matches "${source_text}")

    # Extract test names
    # ----------------------------------------------------------
    foreach(source_match IN LISTS source_matches)
      # CMake regex replacement is used as a lightweight parser for TEST names
      string(REGEX REPLACE "${test_capture_pattern}" "\\1" suite "${source_match}")
      string(REGEX REPLACE "${test_capture_pattern}" "\\2" test_case "${source_match}")
      list(APPEND rktest_cases "${suite}.${test_case}")
    endforeach()
  endforeach()

  # Validate discovery
  # ----------------------------------------------------------
  if(NOT rktest_cases)
    message(FATAL_ERROR "No rktest TEST(suite, case) entries found in unit test sources")
  endif()

  # Return to caller
  # ----------------------------------------------------------
  set("${out_var}" "${rktest_cases}" PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------
# Native rktest Report Helpers
# --------------------------------------------------------------------
# Native report targets are for humans
function(rhio_add_rktest_report target_name filter_pattern)
  add_custom_target(
    "${target_name}"
    COMMAND rhio_unit_tests ${RHIO_RKTEST_REPORT_ARGS} "--rktest_filter=${filter_pattern}"
    DEPENDS rhio_unit_tests
    USES_TERMINAL
    COMMENT "Running ${filter_pattern} with native rktest output"
  )
endfunction()

# --------------------------------------------------------------------
# CTest Entries
# --------------------------------------------------------------------
rhio_collect_rktest_cases(RHIO_RKTEST_CASES ${RHIO_UNIT_TEST_SOURCES})

set(RHIO_RKTEST_SUITES)
foreach(rktest_case IN LISTS RHIO_RKTEST_CASES)
  # Split "suite.case" into the two names expected by rktest filtering
  string(REGEX REPLACE "^([^.]+)\\..*$" "\\1" suite "${rktest_case}")
  string(REGEX REPLACE "^[^.]+\\.(.*)$" "\\1" test_case "${rktest_case}")

  rhio_add_rktest_case("${suite}" "${test_case}")
  list(APPEND RHIO_RKTEST_SUITES "${suite}")
endforeach()

# Suite report targets are generated once per discovered suite
list(REMOVE_DUPLICATES RHIO_RKTEST_SUITES)

# --------------------------------------------------------------------
# Native rktest Report Targets
# --------------------------------------------------------------------
# rhio_unit_report runs all C rktest cases with native rktest output
add_custom_target(
  rhio_unit_report
  COMMAND rhio_unit_tests ${RHIO_RKTEST_REPORT_ARGS}
  DEPENDS rhio_unit_tests
  USES_TERMINAL
  COMMENT "Running all rhio unit tests with native rktest output"
)

# rhio_unit_report_<suite> runs only one rktest suite, for example:
#   cmake --build build-tests --target rhio_unit_report_device
foreach(suite IN LISTS RHIO_RKTEST_SUITES)
  rhio_add_rktest_report("rhio_unit_report_${suite}" "${suite}.*")
endforeach()

# --------------------------------------------------------------------
# C++ Header Smoke Test
# --------------------------------------------------------------------
add_executable(rhio_cpp_include_smoke cpp_include_smoke.cpp)
target_compile_features(rhio_cpp_include_smoke PRIVATE cxx_std_11)
target_link_libraries(rhio_cpp_include_smoke PRIVATE rhio_unit_config)

add_test(NAME rhio.header.cxx COMMAND rhio_cpp_include_smoke)
