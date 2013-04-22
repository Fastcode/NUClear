cmake_minimum_required(VERSION 2.8)
project(NUClear)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compiler settings -- TODO: Add support for non-gcc style flags
set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(GCC_MINIMUM_VERSION 4.7.2)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if(NOT(GCC_VERSION VERSION_GREATER ${GCC_MINIMUM_VERSION} 
        OR GCC_VERSION VERSION_EQUAL ${GCC_MINIMUM_VERSION}))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7.2 or greater.")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else()
    message(fatal_error "Unsupported compiler!")
endif()

# Add the subdirectories
add_subdirectory(src/NUClear)

# Temporary main compilation
add_executable(main main.cpp)
target_link_libraries(main nuclear)
