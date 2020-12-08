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
target_compile_features(
  nuclear
  PUBLIC
  c_std_99
  cxx_constexpr
  cxx_decltype
  cxx_deleted_functions
  cxx_generic_lambdas
  cxx_inheriting_constructors
  cxx_lambdas
  cxx_lambda_init_captures
  cxx_long_long_type
  cxx_noexcept
  cxx_nonstatic_member_init
  cxx_nullptr
  cxx_range_for
  cxx_return_type_deduction
  cxx_right_angle_brackets
  cxx_rvalue_references
  cxx_static_assert
  cxx_thread_local
  cxx_trailing_return_types
  cxx_uniform_initialization
  cxx_variadic_templates
  cxx_template_template_parameters
)

# Enable warnings, and all warnings are errors
if(MSVC)
  target_compile_options(nuclear PRIVATE /W4 /WX)
else()
  target_compile_options(nuclear PRIVATE -Wall -Wextra -pedantic -Werror)
endif(MSVC)

# Make the NUClearConfig files
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
write_basic_package_version_file(${PROJECT_BINARY_DIR}/NUClearConfigVersion.cmake COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/NUClearConfig.cmake.in
  ${PROJECT_BINARY_DIR}/NUClearConfig.cmake
  INSTALL_DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear
  PATH_VARS
  INSTALL_INCLUDE_DIR
)

# Install version, config and target files.
install(
  FILES ${PROJECT_BINARY_DIR}/NUClearConfigVersion.cmake ${PROJECT_BINARY_DIR}/NUClearConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear
)
install(EXPORT nuclear-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NUClear NAMESPACE NUClear::)

# Install headers and targets
install(TARGETS nuclear EXPORT nuclear-targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nuclear_bits
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.ipp"
  PATTERN "*.h"
)
install(FILES ${PROJECT_BINARY_DIR}/nuclear DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS nuclear NAMESPACE NUClear:: FILE nuclear-targets.cmake)
