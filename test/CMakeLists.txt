cmake_minimum_required (VERSION 3.20)

add_subdirectory(attendee)

# Project
project(testattender)

add_executable(testattender "tests.cpp")

target_include_directories(testattender PRIVATE ${CURL_INCLUDE_DIRS} "attendee/include")
target_link_libraries(testattender PRIVATE attender attendee -lgtest)

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3)
target_compile_options(testattender PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(testattender PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

if (PAUSE_AT_TEST_END)
    target_compile_definitions(testattender PRIVATE PAUSE_AT_TEST_END=1)
endif()