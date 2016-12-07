OPTION(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" FALSE)

IF(BUILD_DOCUMENTATION)

    FIND_PACKAGE(Doxygen REQUIRED)
    FIND_PACKAGE(Sphinx REQUIRED)

    IF(NOT DOXYGEN_FOUND)
        MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
    ENDIF()

    SET(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    SET(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    CONFIGURE_FILE(${doxyfile_in} ${doxyfile} @ONLY)

    ADD_CUSTOM_TARGET(doxygen_xml
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation XML using doxygen"
        DEPENDS ${doxyfile}
    )

    ADD_CUSTOM_TARGET(docs
        COMMAND ${Sphinx_BINARY} -a ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation pages using sphinx"
        DEPENDS doxygen_xml
    )

    INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)

ENDIF(BUILD_DOCUMENTATION)
