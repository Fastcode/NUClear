# Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

include(CMakeParseArguments)
function(header_library)
  # Extract the arguments from our function call
  set(options, "")
  set(oneValueArgs "NAME")
  set(multiValueArgs "HEADER" "PATH_SUFFIX" "URL")
  cmake_parse_arguments(PACKAGE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Clear our required_vars variable
  unset(required_vars)
  set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/include")

  # Find our include path
  find_path(
    "${PACKAGE_NAME}_INCLUDE_DIR"
    NAMES ${PACKAGE_HEADER}
    DOC "The ${PACKAGE_NAME} include directory"
    PATHS "${OUTPUT_DIR}"
    PATH_SUFFIXES ${PACKAGE_PATH_SUFFIX}
  )

  # File doesn't exist in standard search paths, download it
  if(NOT ${PACKAGE_NAME}_INCLUDE_DIR)

    # Create the output folder if it doesn't already exist
    if(NOT EXISTS "${OUTPUT_DIR}")
      file(MAKE_DIRECTORY "${OUTPUT_DIR}")
    endif()

    # Download file.
    file(DOWNLOAD "${PACKAGE_URL}" "${OUTPUT_DIR}/${PACKAGE_HEADER}" STATUS ${PACKAGE_NAME}_STATUS)

    list(GET ${PACKAGE_NAME}_STATUS 0 ${PACKAGE_NAME}_STATUS_CODE)
    list(GET ${PACKAGE_NAME}_STATUS 1 ${PACKAGE_NAME}_ERROR_STRING)

    # Parse download status
    if(${PACKAGE_NAME}_STATUS_CODE EQUAL 0)
      message(STATUS "Successfully downloaded ${PACKAGE_NAME} library.")

      set(${PACKAGE_NAME}_INCLUDE_DIR "${OUTPUT_DIR}")

    else()
      message(ERROR "Failed to download ${PACKAGE_NAME} library. Error: ${${PACKAGE_NAME}_ERROR_STRING}")
      file(REMOVE "${OUTPUT_DIR}/${PACKAGE_HEADER}")
    endif()
  endif()

  # Setup and export our variables
  set(required_vars ${required_vars} "${PACKAGE_NAME}_INCLUDE_DIR")
  set(${PACKAGE_NAME}_INCLUDE_DIRS ${${PACKAGE_NAME}_INCLUDE_DIR} PARENT_SCOPE)
  mark_as_advanced(${PACKAGE_NAME}_INCLUDE_DIR ${PACKAGE_NAME}_INCLUDE_DIRS)

  # Find the package
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    ${PACKAGE_NAME}
    FOUND_VAR
    ${PACKAGE_NAME}_FOUND
    REQUIRED_VARS
    ${required_vars}
    VERSION_VAR
    ${PACKAGE_NAME}_VERSION
  )

  # Export our found variable to parent scope
  set(${PACKAGE_NAME}_FOUND ${PACKAGE_NAME}_FOUND PARENT_SCOPE)

endfunction(header_library)
