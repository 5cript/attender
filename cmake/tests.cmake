# Version check
cmake_minimum_required (VERSION 3.20)

add_executable(test-attender ${sources} "${CMAKE_CURRENT_SOURCE_DIR}/test/tests.cpp")

target_link_libraries(test-attender PRIVATE attender -lgtest)