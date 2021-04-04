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
	"${CMAKE_CURRENT_SOURCE_DIR}/Pixi.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Rom.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar/asardll.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/Util.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/JsonData.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Entities.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp"

	"${CMAKE_CURRENT_SOURCE_DIR}/Pixi.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Rom.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar/asardll.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Array.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Util.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Config.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/JsonData.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Entities.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.h"

	# json library
	"${CMAKE_CURRENT_SOURCE_DIR}/json/json.hpp"
	# fmt
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/core.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/format.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/format-inl.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/fmt/color.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/fmt/format.cc"
)
add_executable (Pixi ${PIXI_SOURCE_FILES})
if (CMAKE_COMPILER_IS_GNUCXX)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		message(STATUS "Building debug mode")
		target_compile_definitions(Pixi PUBLIC DEBUG)
		target_compile_options(Pixi PRIVATE -fsanitize=address,leak,undefined)
		target_link_options(Pixi PRIVATE -fsanitize=address,leak,undefined)
	else()
		message(STATUS "Building release mode")
		target_link_options(Pixi PRIVATE -s -Wl,--gc-sections)
	endif()
	message(STATUS "GCC detected, adding compile flags")
	target_link_libraries(Pixi PRIVATE dl)
	target_compile_options(Pixi PRIVATE -Wall -Wextra -Wpedantic)
else()
	message(STATUS "Build type is ${CMAKE_CONFIGURATION_TYPES}")
	if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
		target_compile_definitions(Pixi PUBLIC DEBUG)
	endif()
	set(CMAKE_GENERATOR "Visual Studio 16 2019")
	set(CMAKE_GENERATOR_PLATFORM Win32 CACHE INTERNAL "")
	set(CMAKE_VS_PLATFORM_NAME Win32 CACHE INTERNAL "")
	target_link_options(Pixi PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
	message(STATUS "MSVC detected, adding compile flags")
	target_compile_options(Pixi PRIVATE /MT /Wall /std:c++17)
	target_compile_options(Pixi PRIVATE
			# generic & extremely noisy warnings which do (almost) nothing useful
            # not to say that most come from code which I have no control over
			/wd4514 # unreferenced inline function removed
			/wd4710 # function not inlined
			/wd4711 # function selected for automatic inline expansion
			/wd4820 # 'bytes' bytes padding added after construct 'member_name'
			/wd4464 # relative include path contains '..'

            # specific warnings of pixi
            /wd4996 # deprecated function
            /wd6031 # return value ignored -> should probably be taken care of instead of ignored at some point
            /wd4365 # signed/unsigned mismatch -> should probably be taken care of instead of ignored at some point
            /wd5045 # spectre mitigation
            /wd4774 # format string is not a string literal

            # from here are warnings that I can't do anything about
            # but I still dont't want them clogging my output

            # from nlhomann's json.hpp
            /wd4061 # enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
            /wd4623 # default constructor was implicitely defined as deleted
            /wd4625 # copy constructor was implicitely defined as deleted
            /wd4626 # assignment operator was implicitely defined as deleted
            /wd5027 # move assignment operator was implicitely defined as deleted
            /wd26819 # unannotated fallthrought between switch labels
            /wd28020 # The expression <expr> is not true at this call
            /wd26495 # Variable <var> is uninitialized. Always initialize a member variable

            # from asardll.c
            /wd5039 # throwing stuff passed to extern "C"
            /wd4090 # function different 'const' qualifiers (stuff from asardll.c)

            # from stdlib's limits.h for some reason (why does stuff from the stdlib have warnings)
            /wd4668
	)
endif()

# TODO: Add tests and install targets if needed.
