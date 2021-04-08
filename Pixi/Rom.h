#include "Entities.h"

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
	size_t m_header_offset;
	MapperType m_mapper;
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
	bool patch(Sprite& spr, const std::vector<std::string>& extraDefines, PixiConfig& cfg);
	bool patch(const std::string& dir, const std::string& file, PixiConfig& cfg);
	bool patch(const std::string& file, PixiConfig& cfg);
	void close();
	void run_checks();
};