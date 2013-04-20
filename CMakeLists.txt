cmake_minimum_required(VERSION 2.8)
project(NUClear)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add the subdirectories
add_subdirectory(src/NUClear)
