# CMakeList.txt : CMake project for Pixi, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(PIXI_SOURCE_FILES "")
list(
	APPEND PIXI_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/icon.rc"
	"${CMAKE_CURRENT_SOURCE_DIR}/Pixi.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Rom.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar/asardll.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Util.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/JsonData.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Entities.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpritesData.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/MeiMei/MeiMei.cpp"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/Pixi.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Rom.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar/asardll.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Array.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Util.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/JsonData.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Entities.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/SpritesData.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/MeiMei/MeiMei.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/StructParams.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/MemoryFile.h"
	
	# json library
	"${CMAKE_CURRENT_SOURCE_DIR}/json/json.hpp"
	# fmt
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/core.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/format.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/format-inl.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/color.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/format.cc"
	# toml
	"${CMAKE_CURRENT_SOURCE_DIR}/toml/toml.hpp"
)
add_executable (Pixi ${PIXI_SOURCE_FILES})
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		message(STATUS "Building debug mode")
		target_compile_definitions(Pixi PUBLIC DEBUG)
		target_compile_options(Pixi PRIVATE -fsanitize=address,leak,undefined)
		target_link_options(Pixi PRIVATE -fsanitize=address,leak,undefined)
	else()
		message(STATUS "Building release mode")
		target_link_options(Pixi PRIVATE -s -Wl,--gc-sections)
	endif()
	message(STATUS "GCC/Clang detected, adding compile flags")
	target_link_libraries(Pixi PRIVATE dl)
	target_compile_options(Pixi PRIVATE -Wall -Wextra -Wpedantic)
else()
	message(STATUS "Build type is ${CMAKE_CONFIGURATION_TYPES}")
	if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
		target_compile_definitions(Pixi PUBLIC DEBUG)
		STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
		STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
		target_compile_options(Pixi PRIVATE /fsanitize=address)
	endif()
	set(CMAKE_GENERATOR "Visual Studio 16 2019")
	set(CMAKE_GENERATOR_PLATFORM Win32 CACHE INTERNAL "")
	set(CMAKE_VS_PLATFORM_NAME Win32 CACHE INTERNAL "")
	target_compile_options(Pixi PRIVATE /MT /Wall /std:c++17)
	target_link_options(Pixi PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
	message(STATUS "MSVC detected, adding compile flags")

	target_compile_options(Pixi PRIVATE
			# generic & extremely noisy warnings which do (almost) nothing useful
            # not to say that most come from code which I have no control over
			/wd4514 # unreferenced inline function removed
			/wd4710 # function not inlined
			/wd4820 # 'bytes' bytes padding added after construct 'member_name'
			/wd4464 # relative include path contains '..'
			/wd4100 # unreferenced formal parameter
			/wd4189 # local variable is initialized but not referenced
			#
            ## specific warnings of pixi
            /wd4996 # deprecated function
            /wd4365 # signed/unsigned mismatch -> should probably be taken care of instead of ignored at some point
            /wd5045 # spectre mitigation
			#
            ## from here are warnings that I can't do anything about
            ## but I still dont't want them clogging my output
			#
            ## from nlhomann's json.hpp
            /wd4061 # enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
            /wd4623 # default constructor was implicitely defined as deleted
            /wd4625 # copy constructor was implicitely defined as deleted
            /wd4626 # assignment operator was implicitely defined as deleted
            /wd5027 # move assignment operator was implicitely defined as deleted
			/wd5026 # move constructor was implicitly defined as deleted
			#
            ## from asardll.c
            /wd5039 # throwing stuff passed to extern "C"
            /wd4090 # function different 'const' qualifiers (stuff from asardll.c)
			#
            ## from stdlib's limits.h for some reason (why does stuff from the stdlib have warnings)
            /wd4668
	)
endif()