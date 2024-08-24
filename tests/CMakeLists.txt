#[[
MIT License

Copyright (c) 2013 NUClear Contributors

This file is part of the NUClear codebase.
See https://github.com/Fastcode/NUClear for further info.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
]]

# Get catch2 using FetchContent
include(FetchContent)
FetchContent_Declare(
  Catch2
  SYSTEM DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.tar.gz
)
set(CATCH_CONFIG_CONSOLE_WIDTH 120)
FetchContent_MakeAvailable(Catch2)

# Silence clang-tidy warnings from catch2
get_target_property(catch2_target Catch2::Catch2 ALIASED_TARGET)
set_target_properties(${catch2_target} PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(${catch2_target} PROPERTIES CXX_CLANG_TIDY "")
get_target_property(catch2_with_main_target Catch2::Catch2WithMain ALIASED_TARGET)
set_target_properties(${catch2_with_main_target} PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(${catch2_with_main_target} PROPERTIES CXX_CLANG_TIDY "")

# Create a test_util library that is used by all tests
file(GLOB_RECURSE test_util_src "test_util/*.cpp")
add_library(test_util OBJECT ${test_util_src})
target_link_libraries(test_util INTERFACE "$<LINK_LIBRARY:WHOLE_ARCHIVE,NUClear::nuclear>")
target_link_libraries(test_util PUBLIC Catch2::Catch2)
target_include_directories(test_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(
  test_util SYSTEM PUBLIC ${CATCH_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/include "${PROJECT_SOURCE_DIR}/src"
)

# Create a test binary for each test file
file(GLOB_RECURSE test_sources "tests/*.cpp")
foreach(test_file ${test_sources})
  get_filename_component(test_name ${test_file} NAME_WE)
  get_filename_component(test_dir ${test_file} DIRECTORY)
  file(RELATIVE_PATH test_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests" ${test_dir})

  add_executable(${test_name} ${test_file})
  target_include_directories(${test_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${test_name} test_util)
  target_link_libraries(${test_name} Catch2::Catch2WithMain)

  set_property(TARGET ${test_name} PROPERTY FOLDER "tests/${test_dir}")
  set_property(TARGET ${test_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${test_dir}")

  # Add the test
  add_test(
    NAME "${test_dir}/${test_name}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_dir}/${test_name} --order rand
  )
endforeach()

# Special test binary for testing NUClearNet
add_executable(test_network networktest.cpp)
target_link_libraries(test_network test_util)
target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
