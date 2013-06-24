FIND_PACKAGE(CATCH)
FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)

include_directories(${CATCH_INCLUDE_DIRS})
include_directories(${ZMQ_INCLUDE_DIRS})

FILE(GLOB testsrc "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

source_group(Tests FILES ${testsrc})

add_executable(tests tests.cpp ${testsrc})
target_link_libraries(tests nuclear ${ZMQ_LIBRARIES} ${PROTOBUF_LIBRARY})
add_test(tests tests)
