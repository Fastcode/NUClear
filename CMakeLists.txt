cmake_minimum_required(VERSION 2.8)
project(NUClear)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)

# Supported options:
option(NUCLEAR_BUILD_TESTS "Builds all of the NUClear unit tests." ON)

# We use additional modules that cmake needs to know about
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# This directory contains our header files and ipp files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# Our protocol buffers are generated in this folder
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

# Common C++ Flags
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# XCode support. TODO: Consider better placement for this
if("${CMAKE_GENERATOR}" MATCHES "Xcode")
    message("Enabling xcode support")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(GCC_MINIMUM_VERSION 4.7)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if(GCC_VERSION VERSION_LESS ${GCC_MINIMUM_VERSION})
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7.2 or greater.")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")
    
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -stdlib=libc++")
    
else()
    message(fatal_error "Unsupported compiler!")
endif()

# Add the subdirectories
add_subdirectory(src/NUClear)

# Optional build supports
if(NUCLEAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
