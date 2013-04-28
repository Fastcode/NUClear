find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIR})

add_executable(raw_test raw_test.cpp)
target_link_libraries(raw_test nuclear ${GTEST_BOTH_LIBRARIES})
add_test(raw_tests raw_test)
