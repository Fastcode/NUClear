FIND_PACKAGE(CATCH REQUIRED)
INCLUDE_DIRECTORIES(${CATCH_INCLUDE_DIRS})

FILE(GLOB_RECURSE testsrc "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

FOREACH(test ${testsrc})

    # Get just the file name
    GET_FILENAME_COMPONENT(raw_name ${test} NAME_WE)
    ADD_EXECUTABLE(${raw_name} ${test})
    TARGET_LINK_LIBRARIES(${raw_name} nuclear)
    ADD_TEST(${raw_name} ${raw_name})
ENDFOREACH()
