# Collect all currently added targets in all subdirectories
#
# Parameters:
#  - _result the list containing all found targets
#  - _dir root directory to start looking from
function(get_all_catch_test_targets _result _dir)
  get_property(
    _subdirs
    DIRECTORY "${_dir}"
    PROPERTY SUBDIRECTORIES
  )
  foreach(_subdir IN LISTS _subdirs)
    get_all_catch_test_targets(${_result} "${_subdir}")
  endforeach()

  unset(catch_targets)
  get_directory_property(_sub_targets DIRECTORY "${_dir}" BUILDSYSTEM_TARGETS)
  foreach(target ${_sub_targets})
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "EXECUTABLE")
      get_target_property(target_link_libraries ${target} INTERFACE_LINK_LIBRARIES)
      if(target_link_libraries MATCHES "Catch2::Catch2")
        list(APPEND catch_targets ${target})
      endif()
    endif()
  endforeach()

  set(${_result}
      ${${_result}} ${catch_targets}
      PARENT_SCOPE
  )
endfunction()

# Find all executable targets that link to Catch2::Catch2WithMain || Catch2::Catch2
get_all_catch_test_targets(all_targets ${PROJECT_SOURCE_DIR})

# Create a custom command for each test target to run it
# Make sure that coverage data is written with paths relative to the source directory
unset(report_outputs)
unset(coverage_reports)
foreach(target ${all_targets})

  unset(command_prefix)
  if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Extra output files for the profile data
    set(raw_coverage "${PROJECT_BINARY_DIR}/reports/tests/${target}.profraw")
    set(indexed_coverage "${PROJECT_BINARY_DIR}/reports/tests/${target}.profdata")
    set(coverage_report "${PROJECT_BINARY_DIR}/reports/tests/${target}.txt")
    set(command_prefix "cmake" "-E" "env" "LLVM_PROFILE_FILE=${raw_coverage}")
    list(APPEND coverage_reports ${coverage_report})
  endif()

  set(junit_report_file "${PROJECT_BINARY_DIR}/reports/tests/${target}.junit.xml")
  list(APPEND report_outputs ${junit_report_file})
  add_custom_command(
    OUTPUT ${junit_report_file} ${raw_coverage}
    COMMAND ${command_prefix} $<TARGET_FILE:${target}> --reporter console --reporter JUnit::out=${junit_report_file}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${target}
    USES_TERMINAL
    COMMENT "Running test ${target}"
  )

  if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_custom_command(
      OUTPUT ${indexed_coverage}
      COMMAND llvm-profdata merge -sparse ${raw_coverage} -o ${indexed_coverage}
      DEPENDS ${raw_coverage}
    )

    add_custom_command(
      OUTPUT ${coverage_report}
      COMMAND llvm-cov show --show-branches=count -Xdemangler c++filt
              -instr-profile=${CMAKE_CURRENT_BINARY_DIR}/coverage.profdata $<TARGET_FILE:${target}> > ${coverage_report}
      DEPENDS ${indexed_coverage}
    )
  endif()

endforeach()

# Create a custom target that depends on all test targets
add_custom_target(
  run_all_tests
  DEPENDS ${report_outputs} ${all_targets}
  COMMENT "Running all Catch2 tests"
)

# Concatenate all the coverage reports together
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/reports/coverage.txt
    COMMAND cat ${coverage_reports} > ${PROJECT_BINARY_DIR}/reports/coverage.txt
    DEPENDS ${coverage_reports}
  )

  add_custom_target(
    coverage
    DEPENDS ${PROJECT_BINARY_DIR}/reports/coverage.txt
    COMMENT "Running all Catch2 tests with coverage"
  )

endif()
