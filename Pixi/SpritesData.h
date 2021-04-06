#include "Entities.h"

struct PerLevelData {
	int sprite_ptrs_addr = 0;
	int data_addr = 0;
	ByteArray<uint8_t, 0x400> level_ptrs{};
	ByteArray<uint8_t, 0x4000> sprite_ptrs{};
	ByteArray<uint8_t, 0x8000> data{};
	ByteArray<uint8_t, 0x8000> pls_pointers{};
};

class SpritesData {
	using Svect = std::vector<Sprite>;
private:
	Rom& m_rom;
	Svect m_normal_sprites{};
	Svect m_cluster_sprites{};
	Svect m_extended_sprites{};
	Svect m_ow_sprites{};
	PerLevelData pls_data{};
	std::array<Svect*, FromEnum(ListType::SIZE)> sprites_list{&m_normal_sprites, &m_cluster_sprites, &m_extended_sprites, &m_ow_sprites};
	void patch_sprites(const std::vector<std::string>& extraDefines, Svect& sprites, size_t size, PixiConfig& cfg);
public:
	SpritesData(Rom& rom, const PixiConfig& cfg) : m_rom(rom) {
		m_normal_sprites.reserve(cfg.PerLevel ? Sprite::MAX_SPRITE_COUNT : 0x100);
		m_normal_sprites.insert(m_normal_sprites.begin(), cfg.PerLevel ? Sprite::MAX_SPRITE_COUNT : 0x100, {});
		m_cluster_sprites.reserve(Sprite::SPRITE_COUNT);
		m_cluster_sprites.insert(m_cluster_sprites.begin(), Sprite::SPRITE_COUNT, {});
		m_extended_sprites.reserve(Sprite::SPRITE_COUNT);
		m_extended_sprites.insert(m_extended_sprites.begin(), Sprite::SPRITE_COUNT, {});
		m_ow_sprites.reserve(Sprite::SPRITE_COUNT);
	}

	void populate(PixiConfig& cfg);
	void serialize(const PixiConfig& cfg);
	void write_long_table(Svect::const_iterator spr, const std::string& dir, std::string_view filename);
	bool is_empty_table(Svect::const_iterator spr, int size);
	void patch_sprites_wrap(const std::vector<std::string>& extraDefines, PixiConfig& cfg);

	Svect& operator[](int index) {
		assert(index < FromEnum(ListType::SIZE));
		return *sprites_list[index];
	}

	Svect& operator[](ListType index) {
		assert(index < ListType::SIZE);
		return *sprites_list[FromEnum(index)];
	}

	template <typename T, typename = std::enable_if_t<std::is_same<T, ListType>::value || std::is_same<T, int>::value>>
	Svect& get(T index) {
		return this->operator[](index);
	}

	Svect& normal() {
		return m_normal_sprites;
	}

	Svect& cluster() {
		return m_cluster_sprites;
	}

	Svect& extended() {
		return m_extended_sprites;
	}

	Svect& overworld() {
		return m_ow_sprites;
	}

	Rom& rom() {
		return m_rom;
	}

};