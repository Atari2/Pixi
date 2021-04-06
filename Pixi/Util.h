#ifndef UTIL_INCLUDED_ONCE
#define UTIL_INCLUDED_ONCE
#ifdef WIN32
// there is a stupid max define in windows.h that causes a compilation error
// it's guarded by a #ifndef NOMINMAX, so I define it and away it goes...
#define NOMINMAX 1
#include <windows.h>
#endif
#include <cstdio>
#include "fmt/fmt/format.h"
#include "fmt/fmt/color.h"
#include "asar/asardll.h"
#include "Array.h"
#include <filesystem>

class ErrorState {
	inline static bool asar_inited = false;
public:

	static bool asar_init_wrap() {
		asar_inited = asar_init();
		return asar_inited;
	}
	template <typename ...Args>
	static void pixi_error(const char* format, Args... args) {
#ifndef WIN32
		fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "[ Error ] ");
		fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, format, args...);
#else
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		fmt::print("[ Error ] ");
		fmt::print(format, args...);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

#endif
		if (asar_inited)
			asar_close();
		exit(1);
	}

	template <typename ...Args>
	static void pixi_warning(const char* format, Args... args) {
#ifndef WIN32
		fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "[ Warning ] ");
		fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, format, args...);
#else
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		fmt::print("[ Warning ] ");
		fmt::print(format, args...);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
#endif
	}

	template <typename ...Args>
	static void debug(const char* format, Args... args) {
#ifdef DEBUG
		fmt::print(format, args...);
#endif
	}

};

template <typename T>
constexpr std::underlying_type_t<T> FromEnum(T type) {
	return static_cast<std::underlying_type_t<T>>(type);
}

template <typename T>
void write_all(const T& data, const std::string& full_path, size_t size) {
	FILE* file = fileopen(full_path.c_str(), "wb");
	if (fwrite(data.ptr_at(0), 1, size, file) != size) {
		ErrorState::pixi_error("{} couldn't be fully written. Please check file permissions.", full_path);
	}
	fclose(file);
}

template <typename T>
void write_all(const T& data, const std::string& path, const char* filename, size_t size) {
	write_all<T>(data, path + filename, size);
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
void set_paths_relative_to(std::string& path, std::string_view arg0);
std::string append_to_dir(std::string_view src, std::string_view file);
std::string escapeDefines(std::string_view path, const char* repl = "\\!");
#endif