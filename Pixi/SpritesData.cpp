#include "SpritesData.h"

Sprite& from_table(std::vector<Sprite>& table, int level, int number, bool perlevel, ListType type) {
	static Sprite dummy{ Sprite::INVALID };
	if (type != ListType::Sprite) {
		if (number > Sprite::SPRITE_COUNT)
			return dummy;
		return table[number];
	}
	if (!perlevel)
		return table[number];

	if (level > 0x200 || number > 0xFF)
		return dummy;
	if (level == 0x200)
		return table[0x2000 + number];
	else if (number > 0xB0 && number < 0xC0)
		return table[(level * 0x10) + (number - 0xB0)];
	return dummy;
}

void SpritesData::patch_sprites(const std::vector<std::string>& extraDefines, Svect& sprites, size_t size, PixiConfig& cfg)
{
	for (size_t i = 0; i < size; i++) {
		Sprite& spr = sprites[i];
		if (spr.asm_file.empty())
			continue;
		bool duplicate = false;
		auto res = std::find_if(sprites.rbegin() + (size - i), sprites.rend(), [spr](const Sprite& curr) {
			if (curr.asm_file.empty())
				return false;
			return spr.asm_file == curr.asm_file;
			});
		if (res != sprites.rend()) {
			spr.table.init = (*res).table.init;
			spr.table.init = (*res).table.main;
			spr.extended_cape_ptr = (*res).extended_cape_ptr;
			spr.ptrs = (*res).ptrs;
			duplicate = true;
			break;
		}
		if (!duplicate)
			spr.patch(extraDefines, rom(), cfg);

		if (spr.level < 0x200 && spr.number >= 0xB0 && spr.number < 0xC0) {
			int pls_lv_addr = pls_data.level_ptrs[spr.level * 2] + (pls_data.level_ptrs[spr.level * 2 + 1] << 8);
			if (pls_lv_addr == 0x0000) {
				pls_lv_addr = pls_data.sprite_ptrs_addr + 1;
				pls_data.level_ptrs[spr.level * 2] = (uint8_t)pls_lv_addr;
				pls_data.level_ptrs[spr.level * 2 + 1] = (uint8_t)(pls_lv_addr >> 8);
				pls_data.sprite_ptrs_addr += 0x20;
			}
			pls_lv_addr--;
			pls_lv_addr += (spr.number - 0xB0) * 2;

			if (pls_data.data_addr >= 0x8000)
				ErrorState::pixi_error("Too many Per-Level sprites. Please remove some.\n");

			pls_data.sprite_ptrs[pls_lv_addr] = (uint8_t)(pls_data.data_addr + 1);
			pls_data.sprite_ptrs[pls_lv_addr + 1] = (uint8_t)((pls_data.data_addr + 1) >> 8);

			memcpy(pls_data.data.ptr_at(0) + pls_data.data_addr, &spr.table, 0x10);
			memcpy(pls_data.pls_pointers.ptr_at(0) + pls_data.data_addr, &spr.ptrs, 15);
			int index = pls_data.data_addr + 0x0F;
			if (index < 0x8000) {
				pls_data.pls_pointers[index] = 0xFF;
			}
			else {
				ErrorState::pixi_error("Per-level sprites data address out of bounds of array, value is {}", pls_data.data_addr);
			}
			pls_data.data_addr += 0x10;
		}
	}
}

void SpritesData::populate(PixiConfig& cfg)
{
	auto& list = cfg.m_Paths[PathType::List];
	FILE* listStream = fileopen(list.c_str(), "r");
	uint32_t sprite_id, level;
	int lineno = 0;
	int read_res;
	std::string line;
	ListType type = ListType::Sprite;
	std::string cfgname;
	char cline[1024];

	while (fgets(cline, 1024, listStream) != NULL) {
		line = cline;
		int read_until = -1;
		auto& sprites = get(type);
		lineno++;
		if (line.find(';') != std::string::npos)
			line.erase(line.begin() + line.find_first_of(';'), line.end());
		trim(line);
		if (line.empty())
			continue;
		if (line.find(':') == std::string::npos) {
			level = 0x200;
			read_res = sscanf(line.c_str(), "%x %n", &sprite_id, &read_until);
			if (read_res != 1 || read_res == EOF || read_until == -1)
				ErrorState::pixi_error("Line {} was malformed: \"{}\"\n", lineno, line);
			cfgname = line.substr(read_until);
		}
		else if (line.find(':') == line.length() - 1) {
			if (line == "SPRITE:") {
				type = ListType::Sprite;
			}
			else if (line == "CLUSTER:") {
				type = ListType::Cluster;
			}
			else if (line == "EXTENDED:") {
				type = ListType::Extended;
			}
			else {
				ErrorState::pixi_error("Line {} tried to set a wrong sprite type: {}\n", lineno, line);
			}
			continue;
		}
		else {
			read_res = sscanf(line.c_str(), "%x:%x %n", &level, &sprite_id, &read_until);
			if (read_res != 2 || read_res == EOF || read_until == -1)
				ErrorState::pixi_error("Line {} was malformed: \"{}\"\n", lineno, line);
			if (!cfg.PerLevel)
				ErrorState::pixi_error("Trying to insert per level sprites without using the -pl flag, at line {}: \"{}\"\n", lineno, line);
			cfgname = line.substr(read_until);
		}

		auto dot = cfgname.find_last_of('.');
		if (dot == std::string::npos)
			ErrorState::pixi_error("Error on line {}: missing extension on filename {}\n", lineno, cfgname);
		dot++;

		Sprite& spr = from_table(sprites, level, sprite_id, cfg.PerLevel, type);
		if (spr.invalid) {
			if (type == ListType::Sprite) {
				if (sprite_id >= 0x100)
					ErrorState::pixi_error("Error on line {}: Sprite number must be less than 0x100\n", lineno);
				if (level > 0x200)
					ErrorState::pixi_error("Error on line {}: Level must range from 000-1FF\n", lineno);
				if (sprite_id >= 0xB0 && sprite_id < 0xC0)
					ErrorState::pixi_error("Error on line {}: Only sprite B0-BF must be assigned a level.\n", lineno);
			}
			else {
				if (sprite_id > Sprite::SPRITE_COUNT)
					ErrorState::pixi_error("Error on line {}: Sprite number must be less than {:X}\n", lineno, Sprite::SPRITE_COUNT);
			}
		}
		if (spr.line)
			ErrorState::pixi_error("Error on line {}: Sprite number {:X} already used\n", lineno, sprite_id);
		spr.line = lineno;
		spr.level = level;
		spr.number = sprite_id;
		spr.sprite_type = FromEnum(type);

		if (type != ListType::Sprite) {
			spr.directory = cfg.m_Paths[FromEnum(PathType::Extended) - 1 + FromEnum(type)];
		}
		else {
			if (sprite_id < 0xC0)
				spr.directory = cfg.m_Paths[FromEnum(PathType::Sprites)];
			else if (sprite_id < 0xD0)
				spr.directory = cfg.m_Paths[FromEnum(PathType::Shooters)];
			else
				spr.directory = cfg.m_Paths[FromEnum(PathType::Generators)];
		}
		std::string fullFilename = spr.directory + cfgname;
		std::string ext = cfgname.substr(dot);
		if (type != ListType::Sprite) {
			if (ext != "asm" && ext != "ASM")
				ErrorState::pixi_error("Error on line {}: not an asm file\n", lineno);
			spr.asm_file = fullFilename;
		}
		else {
			spr.cfg_file = fullFilename;
			spr.parse(cfg);
		}

		if (cfg.Debug) {
			fmt::print("Read from line {}\n", spr.line);
			if (spr.level != 0x200)
				fmt::print("Number {:02X} for level {:03X}\n", spr.number, spr.level);
			else
				fmt::print("Number {:02X}\n", spr.number);
			spr.print(stdout);
			fmt::print("\n--------------------------------------\n");
		}

		if (!spr.table.type) {
			spr.table.init = Pointer(Sprite::INIT_PTR + 2 * spr.number);
			spr.table.main = Pointer(Sprite::MAIN_PTR + 2 * spr.number);
		}
	}
}

void SpritesData::serialize(const PixiConfig& cfg)
{
	ErrorState::debug("Try create binary tables\n");
	auto& path = cfg.m_Paths[PathType::Asm];
	write_all(cfg.versionflag, path, "_versionflag.bin", 4);
	if (cfg.PerLevel) {
		write_all(pls_data.level_ptrs, path, "_PerLevelLvlPtrs.bin", 0x400);
		if (pls_data.data_addr == 0) {
			ByteArray<uint8_t, 1> dummy{ 0xFF };
			write_all(dummy, path, "_PerLevelSprPtrs.bin", 1);
			write_all(dummy, path, "_PerLevelT.bin", 1);
			write_all(dummy, path, "_PerLevelCustomPtrTable.bin", 1);
		}
		else {
			write_all(pls_data.sprite_ptrs, path, "_PerLevelSprPtrs.bin", pls_data.sprite_ptrs_addr);
			write_all(pls_data.data, path, "_PerLevelT.bin", pls_data.data_addr);
			write_all(pls_data.pls_pointers, path, "_PerLevelCustomPtrTable.bin", pls_data.data_addr);
			ErrorState::debug("Per-level sprites data size : 0x400+0x{:04X}+2*0x{:04X} = {:04X}\n", pls_data.sprite_ptrs_addr,
				pls_data.data_addr, 0x400 + pls_data.sprite_ptrs_addr + 2 * pls_data.data_addr);
		}
		write_long_table(normal().cbegin() + 0x2000, path, "_DefaultTables.bin");
	}
	else {
		write_long_table(normal().cbegin(), path, "_DefaultTables.bin");
	}

	ByteArray<uint8_t, 0x100 * 15> customstatusptrs{};
	for (int i = 0, j = cfg.PerLevel ? 0x2000 : 0; i < 0x100 * 5; i += 5, j++) {
		memcpy(customstatusptrs.ptr_at(i * 3), &normal()[j].ptrs, 15);
	}
	write_all(customstatusptrs, path, "_CustomStatusPtr.bin", 0x100 * 15);

	ByteArray<uint8_t, Sprite::SPRITE_COUNT * 3> file{};
	for (int i = 0; i < Sprite::SPRITE_COUNT; i++) {
		memcpy(file.ptr_at(i * 3), &cluster()[i].table.main, 3);
	}
	write_all(file, path, "_ClusterPtr.bin", Sprite::SPRITE_COUNT * 3);

	for (int i = 0; i < Sprite::SPRITE_COUNT; i++) {
		memcpy(file.ptr_at(i * 3), &extended()[i].table.main, 3);
	}
	write_all(file, path, "_ExtendedPtr.bin", Sprite::SPRITE_COUNT * 3);
	for (int i = 0; i < Sprite::SPRITE_COUNT; i++) {
		memcpy(file.ptr_at(i * 3), &extended()[i].extended_cape_ptr, 3);
	}
	write_all(file, path, "_ExtendedCapePtr.bin", Sprite::SPRITE_COUNT * 3);

	ErrorState::debug("Binary tables created\n");

	ErrorState::debug("Try create romname files.\n");
	ByteArray<uint8_t, 0x200> extra_bytes{};
	// TODO: implement subfiles, ssc, mwt, mw2, s16
	write_all(extra_bytes, path, "_CustomSize.bin", 0x200);
}

void SpritesData::write_long_table(Svect::const_iterator spr, const std::string& dir, std::string_view filename)
{
	static ByteArray<uint8_t, 0x10> dummy{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	ByteArray<uint8_t, 0x100 * 0x10> file{};
	if (is_empty_table(spr, 0x100)) {
		write_all(dummy, dir, filename.data(), 0x10);
	}
	else {
		for (int i = 0; i < 0x100; i++) {
			memcpy(file.ptr_at(i * 0x10), &spr[i].table, 0x10);
		}
		write_all(file, dir, filename.data(), 0x100 * 0x10);
	}
}

bool SpritesData::is_empty_table(Svect::const_iterator spr, int size) {
	for (int i = 0; i < size; i++) {
		if (!spr[i].table.init.is_empty() || !spr[i].table.main.is_empty())
			return false;
	}
	return true;
}

void SpritesData::patch_sprites_wrap(const std::vector<std::string>& extraDefines, PixiConfig& cfg)
{
	patch_sprites(extraDefines, normal(), normal().size(), cfg);
	patch_sprites(extraDefines, cluster(), cluster().size(), cfg);
	patch_sprites(extraDefines, extended(), extended().size(), cfg);
}

