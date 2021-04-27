#pragma once
#include "Util.h"
#include <cstdint>

class MemoryFile {
	static_assert(sizeof(char) == sizeof(uint8_t) && alignof(char) == alignof(uint8_t));
	enum class DataType {
		Char,
		Byte
	};
	DataType type = DataType::Char;
	bool meimeiKeep = true;
	std::string m_path;
	std::vector<char> m_data;
public:
	static inline bool GlobalKeepFlag = false;
	MemoryFile() = default;
	MemoryFile(const std::string& path, bool keep = true) : meimeiKeep(keep), m_path(path) {
#ifdef WIN32
		// this apparently is only important on windows
		strtolower(m_path);
#endif
	}
	MemoryFile(const MemoryFile& other) = delete;
	MemoryFile& operator=(const MemoryFile& other) = delete;
	MemoryFile(MemoryFile&& other) = delete;
	MemoryFile& operator=(MemoryFile&& other) = delete;

	std::string_view Path() const { return m_path; }
	void SetPath(std::string_view path) { m_path = path; strtolower(m_path); }
	const void* Data() const { return (void*)m_data.data(); }
	size_t Size() const { return m_data.size(); }
	std::back_insert_iterator<std::vector<char>> Inserter() { return std::back_inserter(m_data); };

	template <typename... Args>
	void insertString(std::string_view string, Args... fmt_args) {
		type = DataType::Char;
		fmt::format_to(Inserter(), string, fmt_args...);
	}

	void insertBytes(uint8_t* data, size_t length) {
		type = DataType::Byte;
		m_data.reserve(length);
		m_data.insert(m_data.end(), &data[0], &data[length]);
	}

	void insertBytes(char* data, size_t length) {
		type = DataType::Byte;
		m_data.reserve(length);
		m_data.insert(m_data.end(), &data[0], &data[length]);
	}

	void WriteToFile() {
		FILE* file = nullptr;
		if (type == DataType::Char)
			file = fileopen(m_path.c_str(), "w");
		else
			file = fileopen(m_path.c_str(), "wb");
		fwrite(m_data.data(), sizeof(char), m_data.size(), file);
		fclose(file);
	}

	~MemoryFile() {
		if (GlobalKeepFlag && m_path.length() > 0 && m_data.size() > 0 && meimeiKeep) {
			FILE* file = nullptr;
			if (type == DataType::Char)
				file = fileopen(m_path.c_str(), "w");
			else
				file = fileopen(m_path.c_str(), "wb");
			fwrite(m_data.data(), sizeof(char), m_data.size(), file);
			fclose(file);
		}
	}
};

enum class SpriteFile {
	Version,
	Perlevellvlptrs,
	Perlevelsprptrs,
	Perlevelt,
	Perlevelcustomptrtable,
	Defaulttables,
	Customstatusptr,
	Clusterptr,
	Extendedptr,
	Extendedcapeptr,
	Customsize,
	SIZE
};

class SpriteMemoryFiles {
	std::string m_path;
	MemoryFile version;
	MemoryFile perlevellvlptrs;
	MemoryFile perlevelsprptrs;
	MemoryFile perlevelt;
	MemoryFile perlevelcustomptrtable;
	MemoryFile defaulttables;
	MemoryFile customstatusptr;
	MemoryFile clusterptr;
	MemoryFile extendedptr;
	MemoryFile extendedcapeptr;
	MemoryFile customsize;

	std::array<MemoryFile*, FromEnum(SpriteFile::SIZE)> files{
		&version,
		&perlevellvlptrs,
		&perlevelsprptrs,
		&perlevelt,
		&perlevelcustomptrtable,
		&defaulttables,
		&customstatusptr,
		&clusterptr,
		&extendedptr,
		&extendedcapeptr,
		&customsize,
	};
public:
	SpriteMemoryFiles() = default;

	void SetPath(std::string_view base_path) {
		m_path = base_path;
		version.SetPath(m_path + "_versionflag.bin");
		perlevellvlptrs.SetPath(m_path + "_PerLevelLvlPtrs.bin");
		perlevelsprptrs.SetPath(m_path + "_PerLevelSprPtrs.bin");
		perlevelt.SetPath(m_path + "_PerLevelT.bin");
		perlevelcustomptrtable.SetPath(m_path + "_PerLevelCustomPtrTable.bin");
		defaulttables.SetPath(m_path + "_DefaultTables.bin");
		customstatusptr.SetPath(m_path + "_CustomStatusPtr.bin");
		clusterptr.SetPath(m_path + "_ClusterPtr.bin");
		extendedptr.SetPath(m_path + "_ExtendedPtr.bin");
		extendedcapeptr.SetPath(m_path + "_ExtendedCapePtr.bin");
		customsize.SetPath(m_path + "_CustomSize.bin");
	}

	constexpr MemoryFile& operator[](SpriteFile index) {
		return *files[FromEnum(index)];
	}

};

template <typename T>
void write_all(const T& data, MemoryFile& file, size_t size) {
	file.insertBytes(data.start(), size);
}