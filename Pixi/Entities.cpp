#include "Entities.h"

auto cfg_type(const std::string& line, Sprite& spr) {
	spr.table.type = (uint8_t)std::stoi(line, nullptr, 16);
}

auto cfg_actlike(const std::string& line, Sprite& spr) {
	spr.table.actlike = (uint8_t)std::stoi(line, nullptr, 16);
}

auto cfg_tweak(const std::string& line, Sprite& spr) {
	sscanf(line.c_str(), "%hhx %hhx %hhx %hhx %hhx %hhx", &spr.table.tweak[0], &spr.table.tweak[1], &spr.table.tweak[2],
		&spr.table.tweak[3], &spr.table.tweak[4], &spr.table.tweak[5]);
}

auto cfg_prop(const std::string& line, Sprite& spr) {
	sscanf(line.c_str(), "%hhx %hhx", &spr.table.extra[0], &spr.table.extra[1]);
}
auto cfg_asm(const std::string& line, Sprite& spr) {
	spr.asm_file = append_to_dir(spr.cfg_file, line);
}

std::pair<int, int> read_byte_count(const std::string& line) {
	size_t pos = line.find(':');
	if (pos == std::string::npos) {
		// if there's no ':' it means that this cfg is old, because of backwards compat we just return 0 and ignore
		return { 0, 0 };
	}
	std::pair<int, int> values{};
	try {
		int first = std::stoi(line, nullptr, 16);
		int second = std::stoi(line.substr(pos + 1), nullptr, 16);
		values.first = first;
		values.second = second;
	}
	catch (...) {
		throw std::invalid_argument("Hex values for extra byte count in CFG file where wrongly formatted");
	}
	if (values.first > 12 || values.second > 12) {
		throw std::invalid_argument("Hex value for extra byte count in CFG file out of range, valid range is 00-0C");
	}
	return values;
}

auto cfg_extra(const std::string& line, Sprite& spr) {
	try {
		auto values = read_byte_count(line);
		spr.byte_count = values.first;
		spr.extra_byte_count = values.second;
	}
	catch (const std::invalid_argument& e) {
		ErrorState::pixi_error("While reading cfg file {}: {}\n", spr.cfg_file, e.what());
	}
}

Pointer::Pointer(size_t snes)
{
	lowbyte = (uint8_t)(snes & 0xFF);
	highbyte = (uint8_t)((snes >> 8) & 0xFF);
	bankbyte = (uint8_t)((snes >> 16) & 0xFF);
}

void Sprite::print(FILE* stream)
{
	fmt::print(stream, "Type:       {:02X}\n", table.type);
	fmt::print(stream, "ActLike:    {:02X}\n", table.actlike);
	fmt::print(stream, "Tweak:      {:02X}, {:02X}, {:02X}, {:02X}, {:02X}, {:02X}\n",
		table.tweak[0], table.tweak[1], table.tweak[2], table.tweak[3], table.tweak[4], table.tweak[5]);

	if (table.type) {
		fmt::print(stream, "Extra:      {:02X}, {:02X}\n", table.extra[0], table.extra[1]);
		fmt::print(stream, "ASM File:   {}\n", asm_file);
		fmt::print(stream, "Byte Count: {}, {}\n", byte_count, extra_byte_count);
	}

	if (!displays.empty()) {
		fmt::print(stream, "Displays:\n");
	}
	for (const auto& display : displays) {
		fmt::print(stream, "\tX: {}, Y: {}, Extra-Bit: {}\n", display.x, display.y, display.extra_bit);
		fmt::print(stream, "\tDescription: {}\n", display.description);
		for (const auto& tile : display.tiles) {
			if (tile.text.empty()) {
				fmt::print(stream, "\t\t{},{},{:X}\n", tile.x_offset, tile.y_offset, tile.tile_number);
			}
			else {
				fmt::print(stream, "\t\t{},{},*{}*\n", tile.x_offset, tile.y_offset, tile.text);
			}
		}
	}

	if (!collections.empty()) {
		fmt::print(stream, "Collections:\n");
	}
	for (const auto& collection : collections) {
		std::stringstream coll;
		coll << "\tExtra-Bit: " << (collection.extra_bit ? "true" : "false") << ", Property Bytes: ( ";
		for (int j = 0; j < (collection.extra_bit ? extra_byte_count : byte_count); j++) {
			coll << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(collection.prop[j])
				<< " ";
		}
		coll << ") Name: " << collection.name << std::endl;
		fmt::print(stream, "{}", coll.str());
	}
}

void Sprite::parse(PixiConfig& cfg) {
	std::string extension = cfg_file.substr(cfg_file.find_last_of("."));
	if (extension == ".cfg") {
		from_cfg();
	}
	else if (extension == ".json") {
		from_json(cfg);
	}
	else {
		ErrorState::pixi_error("[ Sprite parsing error ] File extension of {} not one of .cfg or .json, it was {}\n", cfg_file, extension);
	}
}

void Sprite::from_json(PixiConfig& cfg)
{
	nlohmann::json j;
	try {
		std::ifstream instr(cfg_file.c_str());
		if (!instr) {
			ErrorState::pixi_error("\"{}\" wasn't found, make sure to have the correct filenames in your list file\n", cfg_file);
		}
		instr >> j;
	}
	catch (const std::exception& e) {
		if (strstr(e.what(), "parse error") != nullptr) {
			ErrorState::pixi_error("An error was encountered while parsing {}, please make sure that the json file has the correct "
				"format. (error: {})",
				cfg_file, e.what());
		}
		else {
			ErrorState::pixi_error("An unknown error has occurred while parsing {}, please contact the developer providing a "
				"screenshot of this error: {}\n",
				cfg_file, e.what());
		}
	}

	table.actlike = j.at("ActLike");
	table.type = j.at("Type");

	if (table.type) {
		std::string unrooted_asm_file = j.at("AsmFile");
		asm_file = append_to_dir(cfg_file, unrooted_asm_file);

		table.extra[0] = j.at("Extra Property Byte 1");
		table.extra[1] = j.at("Extra Property Byte 2");

		byte_count = j.at("Additional Byte Count (extra bit clear)");
		extra_byte_count = j.at("Additional Byte Count (extra bit set)");
		byte_count = std::clamp(byte_count, 0, 15);
		extra_byte_count = std::clamp(extra_byte_count, 0, 15);
	}

	table.tweak[0] = JsonData<J1656>(j, "$1656").get<uint8_t>();
	table.tweak[1] = JsonData<J1662>(j, "$1662").get<uint8_t>();
	table.tweak[2] = JsonData<J166E>(j, "$166E").get<uint8_t>();
	table.tweak[3] = JsonData<J167A>(j, "$167A").get<uint8_t>();
	table.tweak[4] = JsonData<J1686>(j, "$1686").get<uint8_t>();
	table.tweak[5] = JsonData<J190F>(j, "$190F").get<uint8_t>();

	auto decoded = base64_decode(j.at("Map16"));
	map_data.reserve(decoded.size() / sizeof(Map16));
	for (auto it = decoded.cbegin(); it != decoded.cend(); it += sizeof(Map16)) {
		map_data.push_back({ it });
	}

	displays.reserve(j.at("Displays").size());
	for (const auto& jdisp : j.at("Displays")) {
		Display dis{};
		dis.description = jdisp.at("Description");
		dis.x = jdisp.at("X");
		dis.y = jdisp.at("Y");
		dis.x = std::clamp(dis.x, 0, 0x0F);
		dis.y = std::clamp(dis.y, 0, 0x0F);
		dis.extra_bit = jdisp.at("ExtraBit");

		if (jdisp.at("UseText")) {
			dis.tiles.push_back({});
			dis.tiles[0].text = jdisp.at("DisplayText");
		}
		else {
			dis.tiles.reserve(jdisp.at("Tiles").size());
			for (const auto& jtile : jdisp.at("Tiles")) {
				Tile tile{};
				tile.x_offset = jtile.at("X offset");
				tile.y_offset = jtile.at("Y offset");
				tile.tile_number = jtile.at("map16 tile");
				dis.tiles.push_back(tile);
			}
		}
		displays.push_back(dis);
	}
	collections.reserve(j.at("Collection").size());
	for (const auto& jcoll : j.at("Collection")) {
		Collection coll{};
		coll.name = jcoll.at("Name");
		coll.extra_bit = jcoll.at("ExtraBit");
		for (int i = 1; i <= (coll.extra_bit ? extra_byte_count : byte_count); i++) {
			try {
				coll.prop[i - 1] = jcoll.at("Extra Property Byte " + std::to_string(i));
			}
			catch (const std::out_of_range&) {
				coll.prop[i - 1] = 0;
				cfg.WarningList.push_back("Your json file \"" +
					std::filesystem::path(cfg_file).filename().generic_string() +
					"\" is missing a definition for Extra Property Byte " + std::to_string(i) +
					" at collection \"" + coll.name + "\"");
			}
		}
		collections.push_back(coll);
	}
	DEBUGFMTMSG("Parsed {}\n", cfg_file);
}

void Sprite::from_cfg() {
	constexpr auto linelimit = 6;
	decltype(cfg_type)* handlers[linelimit] = { &cfg_type, &cfg_actlike, &cfg_tweak, &cfg_prop, &cfg_asm, &cfg_extra };
	int nline = 0;
	std::ifstream cfg_stream(cfg_file);
	std::string current_line;
	while (std::getline(cfg_stream, current_line) && nline < linelimit) {
		trim(current_line);
		if (current_line.empty() || current_line.length() == 0)
			continue;
		handlers[nline](current_line, *this);
		nline++;
	}
	DEBUGFMTMSG("Parsed {}, {} lines\n", cfg_file, nline - 1);
}