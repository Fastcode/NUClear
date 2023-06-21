# Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com> 2014-2017 Trent
# Houliston <trent@houliston.me>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.1.0)

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

# Set the project after the build type as the Project command can change the build type
project(
  NUClear
  VERSION 1.0.0
  LANGUAGES C CXX
)

# Determine if NUClear is built as a subproject (using add_subdirectory) or if it is the master project.
set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif()

# If this option is set we are building using continous integration
option(CI_BUILD "Enable build options for building in the CI server" OFF)

# Default not to run the clang-tidy checks, default to whatever our CI_BUILD is
option(ENABLE_CLANG_TIDY "Enable building with clang-tidy checks.")
if(ENABLE_CLANG_TIDY)
  set(CLANG_TIDY_ARGS "clang-tidy" "--use-color")
  if(CI_BUILD)
    set(CLANG_TIDY_ARGS "clang-tidy" "--use-color" "-warnings-as-errors=*")
  endif(CI_BUILD)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c++14")
  set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c99")
endif(ENABLE_CLANG_TIDY)

# If we are doing a CI build then we want to enable -Werror when compiling warnings are bad. We will also make it fail
# if clang-tidy has an error
if(CI_BUILD)
  add_compile_options(-Werror)
endif(CI_BUILD)

# When using valgrind data race detector or helgrind (both are for debugging issues with threading and data races)
# certain symbols need to be overridden to enable better stack traces. This option enables those extra things
# See https://valgrind.org/docs/manual/drd-manual.html#drd-manual.CXX11 for more details
#
# This is disabled by default and will only be enabled when this option is enabled AND NDEBUG is NOT defined (so compiling in debug mode)
option(USE_VALGRIND "Add extra annotations for better valgrind handling" OFF)
if(USE_VALGRIND)
  add_compile_options(-DUSE_VALGRIND)
endif(USE_VALGRIND)

###############################################################################
### Options for enabling different sanitisers.                              ###
###                                                                         ###
### User beware:                                                            ###
### Not all sanitisers can be enabled at the same time.                     ###
###############################################################################
option(USE_ASAN "Enable address sanitization" OFF)
if(USE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=address)
  link_libraries(asan)
endif(USE_ASAN)

option(USE_LSAN "Enable leak sanitization" OFF)
if(USE_LSAN)
  add_compile_options(-fsanitize=leak -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=leak)
  link_libraries(lsan)
endif(USE_LSAN)

option(USE_TSAN "Enable thread sanitization" OFF)
if(USE_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=thread)
  link_libraries(tsan)
endif(USE_TSAN)

option(USE_UBSAN "Enable undefined behaviour sanitization" OFF)
if(USE_UBSAN)
  add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=undefined)
  link_libraries(ubsan)
endif(USE_UBSAN)

# Option for enabling code profiling. Disabled by default
option(ENABLE_PROFILING "Compile with profiling support enabled.")
if(ENABLE_PROFILING)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(
      WARNING
        "Profiling is enabled but no debugging symbols will be kept in the compiled binaries. This may cause fine-grained profilling data to be lost."
    )
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -fprofile-arcs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fprofile-arcs")
  set(CMAKE_LINKER "${CMAKE_LINKER_FLAGS} -pg -fprofile-arcs")
endif(ENABLE_PROFILING)

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
