option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen and Sphinx)" FALSE)

if(BUILD_DOCUMENTATION)

  find_package(Doxygen REQUIRED)
  find_package(Sphinx REQUIRED)

  add_custom_target(
    docs
    COMMAND ${Sphinx_BINARY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation pages using sphinx"
  )

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DESTINATION share/doc OPTIONAL)

endif(BUILD_DOCUMENTATION)
