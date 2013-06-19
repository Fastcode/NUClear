add_executable(raw_test raw_test.cpp)
target_link_libraries(raw_test nuclear)
add_test(raw_tests raw_test)
