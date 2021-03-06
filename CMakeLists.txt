# travis has cmake 3.9
cmake_minimum_required(VERSION 3.9)

project(SAM64)

set(SAM64_CMAKE_DIR "${SAM64_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${SAM64_CMAKE_DIR} ${CMAKE_MODULE_PATH})

enable_testing()

set(PROJECT_NAME "Sam64")
set(PROJECT_Version 0.1)
set(PROJECT_LIBS)

set(COVERAGE OFF CACHE BOOL "Coverage")

set(Boost_USE_STATIC_LIBS        ON)
find_package(Boost REQUIRED COMPONENTS program_options filesystem)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP")
endif()
if (COVERAGE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release"
		FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_UNICODE -DUNICODE)
endif()

set(CORE_SRC_FILES
  source/ram.h
  source/ram.cpp
  )

set(CPU_SRC_FILES
  source/cpu/opcode.h
  source/rom_loader.h
  source/rom_loader.cpp
  source/address_bus.h
  source/direct_address_bus.h
  source/direct_address_bus.cpp
  source/debug_info.h
  source/debug_info.cpp
)
  
set(SRC_FILES
  source/main.cpp
)

add_library(core STATIC ${CORE_SRC_FILES})
add_library(cpu STATIC ${CPU_SRC_FILES})

add_executable(sam64 ${SRC_FILES})
target_link_libraries(sam64 cpu core ${Boost_LIBRARIES})

include(BoostTestHelpers.cmake)
add_boost_test(source/cpu/m6502_test.cpp cpu core)
