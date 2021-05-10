#pragma once
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include "base64/base64.h"
#include "JsonData.h"
#include "Config.h"

enum class ListType : int {
	Sprite,
	Extended,
	Cluster,
	Overworld,
	SIZE
};

struct Pointer {
	static constexpr uint8_t RTL_LOW = 0x21;
	static constexpr uint8_t RTL_HIGH = 0x80;
	static constexpr uint8_t RTL_BANK = 0x01;
	uint8_t lowbyte = RTL_LOW;
	uint8_t highbyte = RTL_HIGH;
	uint8_t bankbyte = RTL_BANK;

	Pointer() = default;
	Pointer(size_t snes);
	Pointer(const Pointer& other);
	Pointer& operator=(const Pointer& other);

	~Pointer() = default;

	bool is_empty() const {
		return lowbyte == RTL_LOW && highbyte == RTL_HIGH && bankbyte == RTL_BANK;
	}

	size_t addr() {
		return (bankbyte << 16) + (highbyte << 8) + lowbyte;
	}
};

struct Tile {
	int x_offset = 0;
	int y_offset = 0;
	int tile_number = 0;
	std::string text{};

	~Tile() = default;
};

struct GfxInfo {
	int gfx_files[4] = { 0x7F, 0x7F, 0x7F, 0x7F };
};

enum class DisplayType {
	XYPosition,
	ExtensionByte
};

struct Display {
	std::string description{};
	std::vector<Tile> tiles{};
	bool extra_bit = false;
	int x_or_index = 0;
	int y_or_value = 0;
	std::vector<GfxInfo> gfx_files{};
	~Display() = default;
};

struct Collection {
	std::string name{};
	bool extra_bit = false;
	uint8_t prop[12] = { 0 };

	~Collection() = default;
};

struct Map8x8 {
	uint8_t tile = 0;
	uint8_t prop = 0;
};

struct Map16 {
	std::array<Map8x8, 4> corners{ { {}, {}, {}, {} } };
	Map16() = default;
	Map16(std::vector<uint8_t>::const_iterator iter) {
		for (int i = 0; i < 4; i++) {
			corners[i].prop = *(iter + (i * 2));
			corners[i].tile = *(iter + (i * 2) + 1);
		}
	}
	Map16(ConstByteIterator<uint8_t> iter) {
		for (int i = 0; i < 4; i++) {
			corners[i].prop = *(iter + (i * 2));
			corners[i].tile = *(iter + (i * 2) + 1);
		}
	}

	constexpr Map8x8& top_left() { return corners[0]; };
	constexpr Map8x8& bottom_left() { return corners[1]; };
	constexpr Map8x8& top_right() { return corners[2]; };
	constexpr Map8x8& bottom_right() { return corners[3]; };
};

struct StatusPointers {
	StatusPointers() = default;
	StatusPointers(const StatusPointers& other);
	StatusPointers& operator=(const StatusPointers& other);

	std::array<Pointer, 5> pointers{ { {}, {}, {}, {}, {} } };
	constexpr Pointer& carriable() {
		return pointers[0];
	};
	constexpr Pointer& kicked() {
		return pointers[1];
	};
	constexpr Pointer& carried() {
		return pointers[2];
	};
	constexpr Pointer& mouth() {
		return pointers[3];
	};
	constexpr Pointer& goal() {
		return pointers[4];
	};
};

struct SpriteTable {
	uint8_t type = 0;
	uint8_t actlike = 0;
	uint8_t tweak[6] = { 0 };
	Pointer init{};
	Pointer main{};
	uint8_t extra[2] = { 0 };
};

struct Sprite {
	static constexpr bool INVALID = true;
	static constexpr int MAX_SPRITE_COUNT = 0x2100;
	static constexpr int SPRITE_COUNT = 0x80;
	static constexpr int INIT_PTR = 0x01817D;
	static constexpr int MAIN_PTR = 0x0185CC;
	static constexpr const char* TEMP_SPR_FILE = "spr_temp.asm";

	bool invalid = false;
	int line = 0;
	int number = 0;
	int level = 0x200;
	SpriteTable table;
	StatusPointers ptrs;
	Pointer extended_cape_ptr;
	int byte_count = 0;
	int extra_byte_count = 0;

	std::string directory{};
	std::string asm_file{};
	std::string cfg_file{};

	std::vector<Map16> map_data{};
	std::vector<Display> displays{};
	std::vector<Collection> collections{};

	DisplayType display_type = DisplayType::XYPosition;
	int sprite_type = 0;
	Sprite() = default;
	Sprite(bool inv) : invalid(inv) {};
	~Sprite() = default;

	void print(FILE*);
	void parse(PixiConfig& cfg);
	void from_json(PixiConfig& cfg);
	void from_cfg();
};