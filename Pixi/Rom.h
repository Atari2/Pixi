#pragma once
#include "Entities.h"

void addIncSrcToFile(MemoryFile& file, const std::vector<std::string>& toInclude);

enum class MapperType : int {
	LoRom, 
	SA1Rom,
	FullSA1Rom
};

inline constexpr std::array<std::string_view, 3> StringMappers{ "LoRom", "SA1Rom", "FullSA1Rom" };

constexpr std::string_view MapperToString(MapperType mapper) {
	return StringMappers[FromEnum(mapper)];
}

class Rom {
	using s = std::numeric_limits<size_t>;
	inline static constexpr size_t MAX_ROM_SIZE = 16 * 1024 * 1024;
	inline static constexpr size_t sa1banks[8] = { 0 << 20, 1 << 20, s::max(), s::max(), 2 << 20, 3 << 20, s::max(), s::max() };
	std::string m_name;
	int m_size = 0;
	ByteArray<uint8_t, MAX_ROM_SIZE> m_data;
	size_t m_header_offset = 0;
	MapperType m_mapper = MapperType::LoRom;
	SpriteMemoryFiles m_main_memory_files{};
	MemoryFile m_shared_patch{};
	MemoryFile m_config_patch{};
public:
	Rom() = default;
	Rom(std::string romname);
	Rom& operator=(Rom&& other) noexcept;
	constexpr MapperType mapper();
	const ByteArrayView<uint8_t> data();

	int& size() { return m_size; }

	// all of these functions access the data directly, without checking for the header
	void write(size_t offset, const uint8_t* data, size_t len);
	void write(size_t offset, const std::vector<uint8_t>& data);
	void write_snes(size_t address, const uint8_t* data, size_t len);
	void write_snes(size_t address, const std::vector<uint8_t>& data);
	uint8_t at(size_t offset);
	uint8_t at_snes(size_t address);

	// these account for the header, the offset is automatically increased by m_header_offset
	int read_long(size_t offset);
	uint16_t read_word(size_t offset);
	uint8_t read_byte(size_t offset);

	// the returned pc address is counted as if it'd index a headered rom
	int pointer_at_snes(size_t address);

	size_t pc_to_snes(size_t address, bool header = true);
	size_t snes_to_pc(size_t address, bool header = true);
	Pointer pointer_snes(int address, int size = 3, int bank = 0x00);
	void clean(PixiConfig& cfg);
	SpriteMemoryFiles& main_memory_files();
	MemoryFile& shared_patch();
	MemoryFile& config_patch();

	// simple classic patch, a single asm file, no virtual memory files
	bool patch_simple(std::string_view path, PixiConfig& cfg);
	
	// patches a single asm file and includes all the binary files that contain the sprite data
	// e.g. pointers, settings, tweaker bytes, extra bytes size, etc.
	bool patch_simple_main(std::string_view path, PixiConfig& cfg);

	// patches a single memory file contaning the wrapper around a sprite
	// also includes shared.asm and config.asm
	bool patch_simple_sprite(MemoryFile& sprite_patch, PixiConfig& cfg, std::string_view spr_name);

	// calls patch_simple_main appending the dir before the filename
	bool patch_main(const std::string& dir, const std::string& file, PixiConfig& cfg);

	// prepares the memory file contaning the wrapper around spr and then calls patch_simple_sprite
	bool patch_sprite(Sprite& spr, const std::vector<std::string>& extraDefines, PixiConfig& cfg);
	
	// generic memoryfile patch
	template <typename... Files>
	bool patch(MemoryFile& file, PixiConfig& cfg, Files&... files) {
		StructParams paramsWrap{ file, files... };
		auto params = paramsWrap.construct(m_data.ptr_at(m_header_offset), MAX_ROM_SIZE, size());
		if (!asar_patch_ex(params)) {
			DEBUGMSG("Failure. Try fetch errors:\n");
			int error_count;
			auto errors = asar_geterrors(&error_count);
			fmt::print("An error has been detected:\n");
			for (int i = 0; i < error_count; i++)
				fmt::print("{}\n", errors[i].fullerrdata);
			ErrorState::pixi_error("An error was encountered in asar\n");
		}
		int warn_count = 0;
		auto loc_warnings = asar_getwarnings(&warn_count);
		for (int i = 0; i < warn_count; i++)
			cfg.WarningList.push_back(loc_warnings[i].fullerrdata);
		DEBUGFMTMSG("Patching for {} successful\n", paramsWrap.PatchLoc());
		return true;
	}

	void close();
	void run_checks();
};