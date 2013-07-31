FIND_PACKAGE(CATCH REQUIRED)

FILE(GLOB testsrc "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

SOURCE_GROUP(Tests FILES ${testsrc})

ADD_EXECUTABLE(tests tests.cpp ${testsrc})
TARGET_LINK_LIBRARIES(tests nuclear)
ADD_TEST(tests tests)
