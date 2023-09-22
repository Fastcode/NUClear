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

find_package(CATCH)

if(CATCH_FOUND)
  # Supported options:
  option(BUILD_TESTS "Builds all of the NUClear unit tests." TRUE)

  if(BUILD_TESTS)
    enable_testing()

    add_compile_definitions(CATCH_CONFIG_CONSOLE_WIDTH=120)

    file(
      GLOB
      test_src
      test.cpp
      "api/*.cpp"
      "dsl/*.cpp"
      "dsl/emit/*.cpp"
      "log/*.cpp"
      "util/*.cpp"
      "util/network/*.cpp"
      "util/serialise/*.cpp"
      "test_util/*.cpp"
    )

    # Some tests must be executed as individual binaries
    file(GLOB individual_tests "${CMAKE_CURRENT_SOURCE_DIR}/individual/*.cpp")

    foreach(test_src ${individual_tests})
      get_filename_component(test_name ${test_src} NAME_WE)

      add_executable(${test_name} ${test_src})
      target_link_libraries(${test_name} NUClear::nuclear)
      set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/individual")
      target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
      target_include_directories(
        ${test_name} SYSTEM PRIVATE ${CATCH_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/include "${PROJECT_SOURCE_DIR}/src"
      )
      # Enable warnings, and all warnings are errors

      add_test(${test_name} test_nuclear)

    endforeach(test_src)

    add_executable(test_nuclear ${test_src})
    target_include_directories(test_nuclear PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_nuclear NUClear::nuclear)
    target_include_directories(
      test_nuclear SYSTEM PRIVATE ${CATCH_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/include "${PROJECT_SOURCE_DIR}/src"
    )
    add_test(test_nuclear test_nuclear)

    add_executable(test_network networktest.cpp)
    target_link_libraries(test_network NUClear::nuclear)
    target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(
      test_network SYSTEM PRIVATE ${CATCH_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/include "${PROJECT_SOURCE_DIR}/src"
    )

  endif(BUILD_TESTS)
endif(CATCH_FOUND)
