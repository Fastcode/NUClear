# Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
#               2014-2017 Trent Houliston <trent@houliston.me>
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

FIND_PACKAGE(CATCH)

IF(CATCH_FOUND)
    # Supported options:
    OPTION(BUILD_TESTS "Builds all of the NUClear unit tests." TRUE)

    IF(BUILD_TESTS)
        ENABLE_TESTING()
        INCLUDE_DIRECTORIES(${CATCH_INCLUDE_DIRS})

        FILE(GLOB_RECURSE testsrc "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

        FILE(GLOB test_base       "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp")
        FILE(GLOB test_api        "${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp")
        FILE(GLOB test_dsl        "${CMAKE_CURRENT_SOURCE_DIR}/dsl/*.cpp")
        FILE(GLOB test_dsl_emit   "${CMAKE_CURRENT_SOURCE_DIR}/dsl/emit/*.cpp")
        FILE(GLOB test_log        "${CMAKE_CURRENT_SOURCE_DIR}/log/*.cpp")

        SOURCE_GROUP(""           FILES ${test_base})
        SOURCE_GROUP(api          FILES ${test_api})
        SOURCE_GROUP(dsl          FILES ${test_dsl})
        SOURCE_GROUP(dsl\\emit    FILES ${test_dsl_emit})
        SOURCE_GROUP(log          FILES ${test_log})

        ADD_EXECUTABLE(test_nuclear ${test_base} ${test_api} ${test_dsl} ${test_dsl_emit} ${test_log})
        TARGET_LINK_LIBRARIES(test_nuclear nuclear)
        ADD_TEST(test_nuclear test_nuclear)

        ADD_EXECUTABLE(test_network networktest.cpp)
        TARGET_LINK_LIBRARIES(test_network nuclear)
        ADD_TEST(test_network test_nuclear)

    ENDIF(BUILD_TESTS)
ENDIF(CATCH_FOUND)
