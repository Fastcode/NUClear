CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(NUClear)

SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 2)
SET(VERSION_PATCH 0)

# Supported options:
OPTION(NUCLEAR_BUILD_TESTS "Builds all of the NUClear unit tests." ON)

# We use additional modules that cmake needs to know about
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# This directory contains our header files and ipp files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
# Our protocol buffers are generated in this folder
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/src)

# Common C++ Flags
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# XCode support
IF("${CMAKE_GENERATOR}" MATCHES "Xcode")
    message("Enabling xcode support")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
ENDIF()

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    SET(GCC_MINIMUM_VERSION 4.7)
    EXECUTE_PROCESS(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    IF(GCC_VERSION VERSION_LESS ${GCC_MINIMUM_VERSION})
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7.2 or greater.")
    ENDIF()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")
    
ELSEIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -stdlib=libc++")
    
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported compiler!")
ENDIF()

# Add the subdirectories
ADD_SUBDIRECTORY(src/NUClear)

# Install NUClear.h
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/NUClear.h DESTINATION include)
# Install the headers and templates that NUClear.h depends on
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/NUClear DESTINATION include PATTERN "*.h" PATTERN "*.ipp")

# Optional build supports
if(NUCLEAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
