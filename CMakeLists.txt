cmake_minimum_required(VERSION 3.17)

# Includes
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake)

# Project
project(attender)

function(add_cxx_compiler_flag FLAG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
endfunction()

if(NOT "${CMAKE_CXX_STANDARD}")
	set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Add files
file(GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# Add library
add_library(attender STATIC ${sources})

find_package(Boost REQUIRED COMPONENTS
	system
	filesystem
)

find_library(LSSL ssl)
find_library(LCRYPTO crypto)
find_library(LCRYPTOPP cryptopp)
find_library(LBROTLI_ENC brotlienc)
find_library(LBROTLI_COMMON brotlicommon)

target_include_directories(attender PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(attender PUBLIC Boost::filesystem Boost::system ${LSSL} ${LCRYPTOPP} ${LCRYPTO} ${LBROTLI_ENC} ${LBROTLI_COMMON} -lstdc++)

# MS SOCK
if (WIN32)
	target_link_libraries(attender PUBLIC -lws2_32 -lmswsock -latomic -lbcrypt)
	
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		target_compile_definitions(attender PUBLIC BOOST_USE_WINDOWS_H=1)
	endif()
else()
endif()

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3)
target_compile_options(attender PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(attender PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

if (ENABLE_TESTING)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
endif()