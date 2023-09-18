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

cmake_minimum_required(VERSION 3.15.0)

# Set the project after the build type as the Project command can change the build type
project(
  NUClear
  VERSION 1.0.0
  LANGUAGES C CXX
)

# We use additional modules that cmake needs to know about
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE
      ${default_build_type}
      CACHE STRING "Choose the type of build." FORCE
  )
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# NUClear targets c++14
set(CMAKE_CXX_STANDARD 14)

# Determine if NUClear is built as a subproject (using add_subdirectory) or if it is the master project.
set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif()

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif(MSVC)

# If this option is set we are building using continous integration
option(CI_BUILD "Enable build options for building in the CI server" OFF)

# Default not to run the clang-tidy checks, default to whatever our CI_BUILD is
option(ENABLE_CLANG_TIDY "Enable building with clang-tidy checks.")
if(ENABLE_CLANG_TIDY)
  # Search for clang-tidy-15 first as this is the version installed in CI
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-15 clang-tidy)
  if(NOT CLANG_TIDY_EXECUTABLE)
    message(FATAL_ERROR "clang-tidy-15 not found.")
  endif()

  # Report clang-tidy executable details
  execute_process(COMMAND "${CLANG_TIDY_EXECUTABLE}" "--version" OUTPUT_VARIABLE CLANG_TIDY_VERSION)
  string(REGEX REPLACE ".*LLVM version ([0-9.]*).*" "\\1" CLANG_TIDY_VERSION "${CLANG_TIDY_VERSION}")
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE} ${CLANG_TIDY_VERSION}")

  # Build clang-tidy command line
  set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}" "--use-color" "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy")
  if(CI_BUILD)
    set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}" "-warnings-as-errors=*")
  endif(CI_BUILD)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c++14")
  set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c99")
endif(ENABLE_CLANG_TIDY)

# If we are doing a CI build then we want to enable -Werror when compiling warnings are bad. We will also make it fail
# if clang-tidy has an error
if(CI_BUILD)
  if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    add_compile_options(/WX)
  else()
    add_compile_options(-Werror)
  endif()
endif(CI_BUILD)

# Make the compiler display colours always (even when we build with ninja)
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
  add_compile_options(-fdiagnostics-color=always)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
  add_compile_options(-fcolor-diagnostics)
endif()

# Add the src directory
add_subdirectory(src)

# Add the tests directory
add_subdirectory(tests)

# Add the documentation subdirectory
add_subdirectory(docs)
