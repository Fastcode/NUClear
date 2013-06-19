FILE(GLOB testsrc "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

source_group(Tests FILES ${testsrc})

add_executable(tests tests.cpp ${testsrc})
target_link_libraries(tests nuclear)
add_test(tests tests)
