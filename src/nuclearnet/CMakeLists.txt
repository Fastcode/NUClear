#[[
MIT License

Copyright (c) 2025 NUClear Contributors

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

# This CMakeLists.txt allows building NUClearNet as a standalone library,
# independent of the NUClear reactor framework.
#
# Usage (standalone):
#   cmake -S src/nuclearnet -B build_net
#   cmake --build build_net
#
# When built as part of the main NUClear project, these sources are included
# automatically via the parent src/CMakeLists.txt glob.

cmake_minimum_required(VERSION 3.15)
project(NUClearNet VERSION 1.0.0 LANGUAGES CXX)

find_package(Threads REQUIRED)

file(GLOB_RECURSE NUCLEARNET_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE NUCLEARNET_HEADERS CONFIGURE_DEPENDS "*.hpp")

add_library(nuclearnet STATIC ${NUCLEARNET_SOURCES})
add_library(NUClear::nuclearnet ALIAS nuclearnet)

target_include_directories(nuclearnet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nuclearnet PUBLIC Threads::Threads)
target_compile_features(nuclearnet PUBLIC cxx_std_14)
set_target_properties(nuclearnet PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Platform-specific linking
if(WIN32)
    target_link_libraries(nuclearnet PUBLIC ws2_32 mswsock iphlpapi)
endif()

# Warnings
if(MSVC)
    target_compile_options(nuclearnet PRIVATE /W4)
else()
    target_compile_options(nuclearnet PRIVATE -Wall -Wextra -pedantic)
endif()
