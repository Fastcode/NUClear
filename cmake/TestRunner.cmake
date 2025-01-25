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
unset(reports)
foreach(target ${all_targets})

  set(sonarqube_report_file "${PROJECT_BINARY_DIR}/reports/tests/${target}.sonarqube.xml")
  set(junit_report_file "${PROJECT_BINARY_DIR}/reports/tests/${target}.junit.xml")
  list(APPEND reports ${sonarqube_report_file} ${junit_report_file})
  add_custom_command(
    OUTPUT ${sonarqube_report_file} ${junit_report_file}
    COMMAND $<TARGET_FILE:${target}> --reporter console --reporter SonarQube::out=${sonarqube_report_file} --reporter
            JUnit::out=${junit_report_file}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    USES_TERMINAL
    COMMENT "Running test ${target}"
  )
endforeach()

# Create a custom target that depends on all test targets
add_custom_target(
  run_all_tests
  DEPENDS ${reports}
  COMMENT "Running all Catch2 tests"
)
