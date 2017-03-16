# Version check
cmake_minimum_required (VERSION 3.0)

# Project
project(attender)

function(add_cxx_compiler_flag FLAG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
endfunction()

IF (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
ENDIF()

# Compiler Options
add_cxx_compiler_flag("-std=c++14")
add_cxx_compiler_flag("-Wall")
add_cxx_compiler_flag("-Wall")
add_cxx_compiler_flag("-Wextra")
add_cxx_compiler_flag("-pedantic")
add_cxx_compiler_flag("-pedantic-errors")
add_cxx_compiler_flag("-Wfloat-equal")
add_cxx_compiler_flag("-fstrict-aliasing")
add_cxx_compiler_flag("-Wstrict-aliasing")
add_cxx_compiler_flag("-O3")

# Add files
file(GLOB sources "attender/*.cpp")

# Add library
add_library(attender STATIC ${sources})

find_library(LBOOST_SYSTEM boost_system)
find_library(LBOOST_FILE_SYSTEM boost_filesystem)
find_library(LATOMIC atomic)
find_library(LSSL ssl)
find_library(LSSL crypto)

set(LIBS "${LBOOST_SYSTEM} ${LBOOST_FILE_SYSTEM} ${LSSL} ${LCRYPTO}")

# MS SOCK
IF (WIN32)
	find_library(LWS2_32 ws2_32)
	find_library(LMSWSOCK mswsock)
	
	set(LIBS "${LIBS} ${LWS2_32} ${LMSWSOCK}")
ELSE()
ENDIF()

MESSAGE(${LIBS})

target_link_libraries(attender PRIVATE "${LIBS}")