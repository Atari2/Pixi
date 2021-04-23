#pragma once
#include "Util.h"

/* giant hack I guess? works thought */
inline bool GlobalKeepFlag = false;
template <typename Unit, typename = std::enable_if_t<sizeof(Unit) == 1>>
class MemoryFile {
	bool meimeiKeep = true;
	std::string m_path;
	std::vector<Unit> m_data;
public:
	MemoryFile() = default;
	MemoryFile(const std::string& path, bool keep = true) : meimeiKeep(keep), m_path(path) {
#ifdef WIN32
		// this apparently is only important on windows
		strtolower(m_path);
#endif
	}
	MemoryFile(const MemoryFile<Unit>& other) = delete;
	MemoryFile<Unit>& operator=(const MemoryFile<Unit>& other) = delete;

	MemoryFile(MemoryFile<Unit>&& other) noexcept {
		m_path = std::move(other.m_path);
		m_data = std::move(other.m_data);
	}

	MemoryFile<Unit>& operator=(MemoryFile<Unit>&& other) noexcept {
		m_path = std::move(other.m_path);
		m_data = std::move(other.m_data);
		return *this;
	}

	std::string_view Path() const { return m_path; }
	const std::vector<Unit>& Data() const { return m_data; }
	std::back_insert_iterator<std::vector<Unit>> Inserter() { return std::back_inserter(m_data); };

	template <typename... Args>
	void insertData(std::string_view dataView, Args... fmt_args) {
		fmt::format_to(Inserter(), dataView, fmt_args...);
	}

	// template <typename = std::enable_if_t<!std::is_same_v<Unit, char>>>
	void insertByteData(Unit* data, size_t length) {
		for (size_t i = 0; i < length; i++)
			m_data.push_back(data[i]);
	}

	/* workaround for meimei, literally the only piece of this tool that mixes uint8_t memoryfiles and char memoryfiles*/
	void insertByteChar(uint8_t* data, size_t length) {
		for (size_t i = 0; i < length; i++)
			m_data.push_back((char)data[i]);
	}

	~MemoryFile() {
		if (GlobalKeepFlag && m_path.length() > 0 && m_data.size() > 0 && meimeiKeep) {
			FILE* file = fileopen(m_path.c_str(), "w");
			fwrite(m_data.data(), sizeof(Unit), m_data.size(), file);
			fclose(file);
		}
	}
};

template <typename T>
void write_all(const T& data, MemoryFile<uint8_t>& file, size_t size) {
	file.insertByteData(data.start(), size);
}