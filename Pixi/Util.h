#ifndef UTIL_INCLUDED_ONCE
#define UTIL_INCLUDED_ONCE
#ifdef WIN32
#include <windows.h>
#undef max
#endif
#include <cstdio>
#include "fmt/fmt/format.h"
#include <filesystem>

template <typename ...Args>
void pixi_error(const char* format, Args... args) {
	fputs("[ Error ] ", stderr);
	fmt::print(stderr, format, args...);
	exit(1);
}

template <typename ...Args>
void pixi_warning(const char* format, Args... args) {
	fputs("[ Warning ] ", stderr);
	fmt::print(stderr, format, args...);
}

template <typename ...Args>
void debug(const char* format, Args... args) {
#ifdef DEBUG
	fmt::print(format, args...);
#endif
}

template <typename T>
constexpr std::underlying_type_t<T> FromEnum(T type) {
	return static_cast<std::underlying_type_t<T>>(type);
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

void wait_before_exit(int arguments);
std::string ask(const char* prompt);
FILE* fileopen(const char* filename, const char* mode);
bool nameEndWithAsmExtension(std::string_view name);
std::string cleanPathTrail(std::string path);
void set_paths_relative_to(std::string& path, const char* arg0);
std::string append_to_dir(std::string_view src, std::string_view file);
#endif