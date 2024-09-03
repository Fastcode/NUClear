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

# Configure the two versions of the main header (in source vs nuclear_bits)
set(nuclear_include_base_directory "")
configure_file(nuclear.in ${PROJECT_BINARY_DIR}/include/nuclear)
set(nuclear_include_base_directory "nuclear_bits/")
configure_file(nuclear.in ${PROJECT_BINARY_DIR}/nuclear)

# Build the library
find_package(Threads REQUIRED)
file(GLOB_RECURSE src "*.c" "*.cpp" "*.hpp" "*.ipp")
add_library(nuclear STATIC ${src})
add_library(NUClear::nuclear ALIAS nuclear)

# Set compile options for NUClear
target_link_libraries(nuclear ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(nuclear PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(nuclear PUBLIC cxx_std_14)

# Enable warnings, and all warnings are errors
if(MSVC)
  target_compile_options(nuclear PRIVATE /W4 /WX)
else()
  target_compile_options(nuclear PRIVATE -Wall -Wextra -pedantic)
endif(MSVC)

# Make the NUClearConfig files
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
write_basic_package_version_file(${PROJECT_BINARY_DIR}/NUClearConfigVersion.cmake COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/NUClearConfig.cmake.in ${PROJECT_BINARY_DIR}/NUClearConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear
  PATH_VARS INSTALL_INCLUDE_DIR
)

# Install version, config and target files.
install(FILES ${PROJECT_BINARY_DIR}/NUClearConfigVersion.cmake ${PROJECT_BINARY_DIR}/NUClearConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear
)
install(
  EXPORT nuclear-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear
  NAMESPACE NUClear::
)

# Install headers and targets
install(
  TARGETS nuclear
  EXPORT nuclear-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nuclear_bits
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.ipp"
  PATTERN "*.h"
)
install(FILES ${PROJECT_BINARY_DIR}/nuclear DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(
  TARGETS nuclear
  NAMESPACE NUClear::
  FILE nuclear-targets.cmake
)
