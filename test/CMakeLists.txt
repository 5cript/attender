cmake_minimum_required (VERSION 3.20)

# Project
project(testattender)

add_executable(testattender ${sources} "${CMAKE_CURRENT_SOURCE_DIR}/test/tests.cpp")

find_package(CURL REQUIRED)

target_include_directories(testattender PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(testattender PRIVATE attender -lgtest CURL::libcurl)

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3)
target_compile_options(testattender PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(testattender PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")