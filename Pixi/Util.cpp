#include "Util.h"

#ifdef WIN32
void double_click_exit() {
    puts("Press any key to exit...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    getc(stdin);
}
#endif

void wait_before_exit(int arguments)
{
#ifdef WIN32
	if (arguments < 2) {
		atexit(double_click_exit);
	}
#endif
}

std::string ask(const char* prompt) {
	char buffer[1024];
	puts(prompt);
	fgets(buffer, 1023, stdin);
	auto len = strlen(buffer);
	while (isspace(buffer[len - 1])) {
		buffer[--len] = '\0';
	}
	if (buffer[0] == '"' && buffer[len - 1] == '"') {
		buffer[len - 1] = '\0';
		memmove(buffer, buffer + 1, len + 1);
	}
	return std::string(buffer);
}

FILE* fileopen(const char* filename, const char* mode) {
	FILE* fp = fopen(filename, mode);
	if (fp == nullptr) {
		ErrorState::pixi_error("Couldn't open {} in mode {}\n", filename, mode);
	}
	return fp;
}

bool nameEndWithAsmExtension(std::string_view name) {
    if (name.size() < 4) return false;
    return name.compare(name.size() - 4, 4, ".asm") == 0 && name[0] != '.';
}

std::string cleanPathTrail(std::string path) {
    if (path.back() == '/' || path.back() == '\\')
        path.pop_back();
    return path;
}

void set_paths_relative_to(std::string& path, std::string_view arg0) {

    if (path.empty())
        return;

    std::filesystem::path absBasePath(std::filesystem::absolute(arg0));
    absBasePath.remove_filename();
    ErrorState::debug("Absolute base path: {} ", absBasePath.generic_string());
    std::filesystem::path filePath(path);
    std::string newPath{};
    if (filePath.is_relative()) {
        newPath = absBasePath.generic_string() + filePath.generic_string();
    }
    else {
        newPath = filePath.generic_string();
    }
    ErrorState::debug("{}\n", newPath);
    if (std::filesystem::is_directory(newPath) && newPath.back() != '/') {
        path = newPath + "/";
    }
    else {
        path = newPath;
    }
}

std::string append_to_dir(std::string_view src, std::string_view file) {
    auto unix_end = src.find_last_of('/');
    auto win_end = src.find_last_of('\\');
    auto end = std::string::npos;
    if (unix_end != std::string::npos && win_end != std::string::npos) {
        end = std::max(unix_end, win_end);
    }
    else if (unix_end != std::string::npos) {
        end = unix_end;
    }
    else if (win_end != std::string::npos) {
        end = win_end;
    }
    // fetches path of src and append it before
    size_t len = end == std::string::npos ? 0 : end + 1;
    std::string new_file = std::string(src.substr(0, len)) + std::string(file);
    std::replace(new_file.begin(), new_file.end(), '\\', '/');
    return new_file;
}

std::string escapeDefines(std::string_view path, const char* repl) {
    std::stringstream ss("");
    for (char c : path) {
        if (c == '!') {
            ss << repl;
        }
        else {
            ss << c;
        }
    }
    return ss.str();
}

FILE* open_subfile(const std::string& name, const char* ext, const char* mode)
{
    std::string filename = name.substr(0, name.find_last_of('.') + 1) + ext;
    return fileopen(filename.c_str(), mode);
}

size_t filesize(FILE* fp)
{
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

bool ends_with(const char* str, const char* suffix) {
    if (str == nullptr || suffix == nullptr)
        return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}