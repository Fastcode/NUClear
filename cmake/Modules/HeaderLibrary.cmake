# Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

INCLUDE(CMakeParseArguments)
FUNCTION(HeaderLibrary)
    # Extract the arguments from our function call
    SET(options, "")
    SET(oneValueArgs "NAME")
    SET(multiValueArgs "HEADER" "PATH_SUFFIX" "URL")
    CMAKE_PARSE_ARGUMENTS(PACKAGE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Clear our required_vars variable
    UNSET(required_vars)

    # Find our include path
    FIND_PATH("${PACKAGE_NAME}_INCLUDE_DIR"
              NAMES ${PACKAGE_HEADER}
              DOC "The ${PACKAGE_NAME} include directory"
              PATHS "${OUTPUT_DIR}/${PACKAGE_HEADER}"
              PATH_SUFFIXES ${PACKAGE_PATH_SUFFIX}
    )

    # File doesn't exist in standard search paths, download it
    IF(NOT ${PACKAGE_NAME}_INCLUDE_DIR)
        SET(OUTPUT_DIR "${CMAKE_BINARY_DIR}/include")

        # Create the output folder if it doesn't already exist
        IF(NOT EXISTS "${OUTPUT_DIR}")
            FILE(MAKE_DIRECTORY "${OUTPUT_DIR}")
        ENDIF()

        # Download file.
        FILE(DOWNLOAD "${PACKAGE_URL}" "${OUTPUT_DIR}/${PACKAGE_HEADER}" STATUS ${PACKAGE_NAME}_STATUS)

        LIST(GET ${PACKAGE_NAME}_STATUS 0 ${PACKAGE_NAME}_STATUS_CODE)

        # Parse download status
        IF(${PACKAGE_NAME}_STATUS_CODE EQUAL 0)
            MESSAGE(STATUS "Successfully downloaded ${PACKAGE_NAME} library.")

            SET(${PACKAGE_NAME}_INCLUDE_DIR "${OUTPUT_DIR}")

        ELSE()
            MESSAGE(ERROR "Failed to download ${PACKAGE_NAME} library.")
        ENDIF()
    ENDIF()

    # Setup and export our variables
    SET(required_vars ${required_vars} "${PACKAGE_NAME}_INCLUDE_DIR")
    SET(${PACKAGE_NAME}_INCLUDE_DIRS ${${PACKAGE_NAME}_INCLUDE_DIR} PARENT_SCOPE)
    MARK_AS_ADVANCED(${PACKAGE_NAME}_INCLUDE_DIR ${PACKAGE_NAME}_INCLUDE_DIRS)

    # Find the package
    INCLUDE(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(${PACKAGE_NAME}
                                      FOUND_VAR ${PACKAGE_NAME}_FOUND
                                      REQUIRED_VARS ${required_vars}
                                      VERSION_VAR ${PACKAGE_NAME}_VERSION
    )

    # Export our found variable to parent scope
    SET(${PACKAGE_NAME}_FOUND ${PACKAGE_NAME}_FOUND PARENT_SCOPE)

ENDFUNCTION(HeaderLibrary)

