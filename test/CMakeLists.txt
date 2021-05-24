cmake_minimum_required (VERSION 3.20)

add_subdirectory(attendee)

# Project
project(testattender)

add_executable(testattender "tests.cpp")

find_package(Boost 1.75.0 REQUIRED
    COMPONENTS
        filesystem
        system
)

target_include_directories(testattender PRIVATE ${CURL_INCLUDE_DIRS} "attendee/include")
target_link_libraries(
    testattender PRIVATE 
    attender attendee -lgtest Boost::filesystem Boost::system
)

# Compiler Options
target_compile_options(testattender PUBLIC -fexceptions -g -O0 -Wall -pedantic-errors -pedantic)

if (PAUSE_AT_TEST_END)
    target_compile_definitions(testattender PRIVATE PAUSE_AT_TEST_END=1)
endif()

add_custom_target(
    node-test-builder ALL
    COMMAND npm install && npm run build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/websocket/node_ws
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/websocket/node_ws/build/echo_server.js node_ws/echo_server.js COPYONLY)