#include "Rom.h"

Rom::Rom(std::string romname) : m_name(romname)
{
	m_data.from_file(fileopen(romname.c_str(), "rb"));
	m_header_offset = m_data.size() & 0x7FFF;
	m_size = m_data.size() - m_header_offset;
	if (data()[0x7fd5] == 0x23) {
		if (data()[0x7fd7] == 0x0D) {
			m_mapper = MapperType::FullSA1Rom;
		}
		else {
			m_mapper = MapperType::SA1Rom;
		}
	}
	else {
		m_mapper = MapperType::LoRom;
	}
	DEBUGFMTMSG("Correctly instantiated rom \"{}\" with mapper {} and header offset {:X}\n", m_name, MapperToString(m_mapper), m_header_offset);
}

Rom& Rom::operator=(Rom&& other) noexcept
{
	m_data = std::move(other.m_data);
	m_name = std::move(other.m_name);
	m_header_offset = other.m_header_offset;
	m_mapper = other.m_mapper;
	m_size = other.m_size;
	return *this;
}

constexpr MapperType Rom::mapper()
{
	return m_mapper;
}

// returns a read only view of the headerless data of the rom
const ByteArrayView<uint8_t> Rom::data()
{
	return ByteArrayView(m_data, m_header_offset);
}

void Rom::write(size_t offset, const uint8_t* data, size_t len) {
	m_data.write_at(data, len, offset);
}

void Rom::write(size_t offset, const std::vector<uint8_t>& data) {
	m_data.write_at(data.data(), data.size(), offset);
}

void Rom::write_snes(size_t address, const uint8_t* data, size_t len) {
	m_data.write_at(data, len, snes_to_pc(address));
}

void Rom::write_snes(size_t address, const std::vector<uint8_t>& data) {
	m_data.write_at(data.data(), data.size(), snes_to_pc(address));
}

uint8_t Rom::at(size_t offset)
{
	return m_data[offset];
}

uint8_t Rom::at_snes(size_t address)
{
	return m_data[snes_to_pc(address)];
}

uint8_t Rom::read_byte(size_t offset) {
	return m_data[m_header_offset + offset];
}

uint16_t Rom::read_word(size_t offset) {
	offset += m_header_offset;
	return m_data[offset] | m_data[offset + 1] << 8;
}

int Rom::read_long(size_t offset) {
	offset += m_header_offset;
	return m_data[offset] | m_data[offset + 1] << 8 | m_data[offset + 2] << 16;
}

int Rom::pointer_at_snes(size_t address) {
	auto offset = snes_to_pc(address);
	return m_data[offset] << 16 | m_data[offset + 1] << 8 | m_data[offset + 2];
}

size_t Rom::pc_to_snes(size_t address, bool header) {
	if (header)
		address -= m_header_offset;

	if (mapper() == MapperType::LoRom) {
		return ((address << 1) & 0x7F0000) | (address & 0x7FFF) | 0x8000;
	}
	else if (mapper() == MapperType::SA1Rom) {
		for (int i = 0; i < 8; i++) {
			if (sa1banks[i] == (address & 0x700000)) {
				return 0x008000 | (i << 21) | ((address & 0x0F8000) << 1) | (address & 0x7FFF);
			}
		}
	}
	else if (mapper() == MapperType::FullSA1Rom) {
		if ((address & 0x400000) == 0x400000) {
			return address | 0xC00000;
		}
		if ((address & 0x600000) == 0x000000) {
			return ((address << 1) & 0x3F0000) | 0x8000 | (address & 0x7FFF);
		}
		if ((address & 0x600000) == 0x200000) {
			return 0x800000 | ((address << 1) & 0x3F0000) | 0x8000 | (address & 0x7FFF);
		}
	}
	return s::max();
}

size_t Rom::snes_to_pc(size_t address, bool header) {

	if (mapper() == MapperType::LoRom) {
		if ((address & 0xFE0000) == 0x7E0000 || (address & 0x408000) == 0x000000 || (address & 0x708000) == 0x700000)
			return s::max();
		address = (address & 0x7F0000) >> 1 | (address & 0x7FFF);
	}
	else if (mapper() == MapperType::SA1Rom) {
		if ((address & 0x408000) == 0x008000) {
			address = sa1banks[(address & 0xE00000) >> 21] | ((address & 0x1F0000) >> 1) | (address & 0x007FFF);
		}
		else if ((address & 0xC00000) == 0xC00000) {
			address = sa1banks[((address & 0x100000) >> 20) | ((address & 0x200000) >> 19)] | (address & 0x0FFFFF);
		}
		else {
			address = s::max();
		}
	}
	else if (mapper() == MapperType::FullSA1Rom) {
		if ((address & 0xC00000) == 0xC00000) {
			address = (address & 0x3FFFFF) | 0x400000;
		}
		else if ((address & 0xC00000) == 0x000000 || (address & 0xC00000) == 0x800000) {
			if ((address & 0x008000) == 0x000000)
				return s::max();
			address = (address & 0x800000) >> 2 | (address & 0x3F0000) >> 1 | (address & 0x7FFF);
		}
		else {
			return s::max();
		}
	}
	else {
		return s::max();
	}

	return address + (header ? m_header_offset : 0);
}

void Rom::clean(PixiConfig& cfg)
{
	if (!strncmp((char*)m_data.ptr_at(snes_to_pc(0x02FFE2)), "STSD", 4)) { // already installed load old tables

		MemoryFile clean_patch{ cfg.AsmDir + "_cleanup.asm" };

		auto version = at(snes_to_pc(0x02FFE6));
		auto flags = at(snes_to_pc(0x02FFE7));

		bool per_level_sprites_inserted = ((flags & 0x01) == 1) || (version < 2);

		// bit 0 = per level sprites inserted
		if (per_level_sprites_inserted) {
			// remove per level sprites
			// version 1.30+
			if (version >= 30) {
				clean_patch.insertString(";Per-Level sprites\n");
				int level_table_address = pointer_snes(0x02FFF1).addr();
				if (level_table_address != 0xFFFFFF && level_table_address != 0x000000) {
					auto pls_addr = snes_to_pc(level_table_address);
					for (int level = 0; level < 0x0400; level += 2) {
						int pls_lv_addr = (m_data[pls_addr + level] + (m_data[pls_addr + level + 1] << 8));
						if (pls_lv_addr == 0)
							continue;
						pls_lv_addr = snes_to_pc(pls_lv_addr + level_table_address);
						for (int i = 0; i < 0x20; i += 2) {
							auto pls_data_addr = (m_data[pls_lv_addr + i] + (m_data[pls_lv_addr + i + 1] << 8));
							if (pls_data_addr == 0)
								continue;
							Pointer main_pointer = pointer_at_snes(pls_data_addr + level_table_address + 0x0B);
							if (main_pointer.addr() == 0xFFFFFF) {
								continue;
							}
							if (!main_pointer.is_empty()) {
								clean_patch.insertString("autoclean ${:06X}\t;{:03X}:{:02X}\n", main_pointer.addr(), level >> 1,
									0xB0 + (i >> 1));
							}
						}
					}
				}
				// version 1.2x
			}
			else {
				for (int bank = 0; bank < 4; bank++) {
					int level_table_address = (m_data[snes_to_pc(0x02FFEA + bank)] << 16) + 0x8000;
					if (level_table_address == 0xFF8000)
						continue;
					clean_patch.insertString(";Per Level sprites for levels {:03X} - {:03X}\n", (bank * 0x80),
						((bank + 1) * 0x80) - 1);
					for (int table_offset = 0x0B; table_offset < 0x8000; table_offset += 0x10) {
						Pointer main_pointer = pointer_snes(level_table_address + table_offset);
						if (main_pointer.addr() == 0xFFFFFF) {
							clean_patch.insertString(";Encountered pointer to 0xFFFFFF, assuming there to be no sprites to clean!\n");
							break;
						}
						if (!main_pointer.is_empty()) {
							clean_patch.insertString("autoclean ${:06X}\n", main_pointer.addr());
						}
					}
					clean_patch.insertString("\n");
				}
			}
		}

		// if per level sprites are inserted, we only have 0xF00 bytes of normal sprites
		// due to 10 bytes per sprite and B0-BF not being in the table.
		// but if version is 1.30 or higher, we have 0x1000 bytes.
		const int limit = version >= 30 ? 0x1000 : (per_level_sprites_inserted ? 0xF00 : 0x1000);

		// remove global sprites
		clean_patch.insertString(";Global sprites: \n");
		int global_table_address = pointer_snes(0x02FFEE).addr();
		if (pointer_snes(global_table_address).addr() != 0xFFFFFF) {
			for (int table_offset = 0x08; table_offset < limit; table_offset += 0x10) {
				Pointer init_pointer = pointer_snes(global_table_address + table_offset);
				if (!init_pointer.is_empty()) {
					clean_patch.insertString("autoclean ${:06X}\n", init_pointer.addr());
				}
				Pointer main_pointer = pointer_snes(global_table_address + table_offset + 3);
				if (!main_pointer.is_empty()) {
					clean_patch.insertString("autoclean ${:06X}\n", main_pointer.addr());
				}
			}
		}

		// remove global sprites' custom pointers
		clean_patch.insertString(";Global sprite custom pointers: \n");
		int pointer_table_address = pointer_snes(0x02FFFD).addr();
		if (pointer_table_address != 0xFFFFFF && pointer_snes(pointer_table_address).addr() != 0xFFFFFF) {
			for (int table_offset = 0; table_offset < 0x100 * 15; table_offset += 3) {
				Pointer ptr = pointer_snes(pointer_table_address + table_offset);
				if (!ptr.is_empty() && ptr.addr() != 0) {
					clean_patch.insertString("autoclean ${:06X}\n", ptr.addr());
				}
			}
		}

		// shared routines
		clean_patch.insertString("\n\n;Routines:\n");
		for (int i = 0; i < 100; i++) {
			int routine_pointer = pointer_snes(0x03E05C + i * 3).addr();
			if (routine_pointer != 0xFFFFFF) {
				clean_patch.insertString("autoclean ${:06X}\n", routine_pointer);
				clean_patch.insertString("\torg ${:06X}\n", 0x03E05C + i * 3);
				clean_patch.insertString("\tdl $FFFFFF\n");
			}
		}

		// Version 1.01 stuff:
		if (version >= 1) {

			// remove cluster sprites
			clean_patch.insertString("\n\n;Cluster:\n");
			int cluster_table = pointer_snes(0x00A68A).addr();
			if (cluster_table != 0x9C1498) // check with default/uninserted address
				for (int i = 0; i < Sprite::SPRITE_COUNT; i++) {
					Pointer cluster_pointer = pointer_snes(cluster_table + 3 * i);
					if (!cluster_pointer.is_empty())
						clean_patch.insertString("autoclean ${:06X}\n", cluster_pointer.addr());
				}

			// remove extended sprites
			clean_patch.insertString("\n\n;Extended:\n");
			int extended_table = pointer_snes(0x029B1F).addr();
			if (extended_table != 0x176FBC) // check with default/uninserted address
				for (int i = 0; i < Sprite::SPRITE_COUNT; i++) {
					Pointer extended_pointer = pointer_snes(extended_table + 3 * i);
					if (!extended_pointer.is_empty())
						clean_patch.insertString("autoclean ${:06X}\n", extended_pointer.addr());
				}
		}
		// everything else is being cleaned by the main patch itself.
		patch(clean_patch, cfg);
	}
	else if (!strncmp((char*)m_data.ptr_at(snes_to_pc(pointer_snes(0x02A963 + 1).addr() - 3)), "MDK", 3)) {
		ErrorState::pixi_error("It looks like this ROM has been patched with Romi's SpriteTool, Pixi is not compatible with it, insertion aborted\n");
	}
}

void addIncSrcToFile(MemoryFile& file, const std::vector<std::string>& toInclude) {
	for (std::string const& incPath : toInclude) {
		file.insertString("incsrc \"{}\"\n", incPath);
	}
}

Pointer Rom::pointer_snes(int address, int size, int bank)
{
	auto offset = snes_to_pc(address);
	auto res = (m_data[offset]) | (m_data[offset + 1] << 8) | (m_data[offset + 2] << 16) * (size - 2);
	res |= (bank << 16);
	return Pointer(res);
}

bool Rom::patch_simple(std::string_view path, PixiConfig& cfg)
{
	if (!asar_patch(path.data(), (char*)m_data.ptr_at(m_header_offset), MAX_ROM_SIZE, &size())) {
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
	DEBUGFMTMSG("Patching for {} successful\n", path);
	return true;
}

bool Rom::patch_simple_main(std::string_view path, PixiConfig& cfg) {
	StructParams paramsWrap{ m_main_memory_files[SpriteFile::Version],
							 m_main_memory_files[SpriteFile::Perlevellvlptrs],
							 m_main_memory_files[SpriteFile::Perlevelsprptrs],
							 m_main_memory_files[SpriteFile::Perlevelt],
							 m_main_memory_files[SpriteFile::Perlevelcustomptrtable],
							 m_main_memory_files[SpriteFile::Defaulttables],
							 m_main_memory_files[SpriteFile::Customstatusptr],
							 m_main_memory_files[SpriteFile::Clusterptr],
							 m_main_memory_files[SpriteFile::Extendedptr],
							 m_main_memory_files[SpriteFile::Extendedcapeptr],
							 m_main_memory_files[SpriteFile::Customsize]
							};
	auto params = paramsWrap.construct(path, m_data.ptr_at(m_header_offset), MAX_ROM_SIZE, size());
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
	DEBUGFMTMSG("Patching for {} successful\n", path);
	return true;
}

MemoryFile& Rom::shared_patch()
{
	return m_shared_patch;
}

MemoryFile& Rom::config_patch()
{
	return m_config_patch;
}

SpriteMemoryFiles& Rom::main_memory_files() {
	return m_main_memory_files;
}

bool Rom::patch_simple_sprite(MemoryFile& sprite_patch, PixiConfig& cfg, std::string_view spr_name)
{
	StructParams paramsWrap(m_config_patch, m_shared_patch);
	auto params = paramsWrap.construct(sprite_patch, m_data.ptr_at(m_header_offset), MAX_ROM_SIZE, size());
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
	DEBUGFMTMSG("Patching for {} successful\n", spr_name);
	return true;
}


bool Rom::patch_main(const std::string& dir, const std::string& file, PixiConfig& cfg) {
	std::string path = dir + file;
	return patch_simple_main(path, cfg);
}

bool Rom::patch_sprite(Sprite& spr, const std::vector<std::string>& extraDefines, PixiConfig& cfg) {
	std::string escapedDir = escapeDefines(spr.directory);
	std::string escapedAsmFile = escapeDefines(spr.asm_file);
	std::string escapedAsmDir = escapeDefines(cfg.AsmDir);
	MemoryFile sprite_patch{ Sprite::TEMP_SPR_FILE };
	sprite_patch.insertString("namespace nested on\n");
	sprite_patch.insertString("incsrc \"{}sa1def.asm\"\n", escapedAsmDir);
	addIncSrcToFile(sprite_patch, extraDefines);
	sprite_patch.insertString("incsrc \"shared.asm\"\n");
	sprite_patch.insertString("incsrc \"{}_header.asm\"\n", escapedDir);
	sprite_patch.insertString("freecode cleaned\n");
	sprite_patch.insertString("SPRITE_ENTRY_{}:\n", spr.number);
	sprite_patch.insertString("\tincsrc \"{}\"\n", escapedAsmFile);
	sprite_patch.insertString("namespace nested off\n");

	bool retval = patch_simple_sprite(sprite_patch, cfg, spr.asm_file);
	std::map<std::string, int> ptr_map = {
		std::pair<std::string, int>("init", 0x018021),
		std::pair<std::string, int>("main", 0x018021),
		std::pair<std::string, int>("cape", 0x000000),
		std::pair<std::string, int>("mouth", 0x000000),
		std::pair<std::string, int>("kicked", 0x000000),
		std::pair<std::string, int>("carriable", 0x000000),
		std::pair<std::string, int>("carried", 0x000000),
		std::pair<std::string, int>("goal", 0x000000)
	};
	int print_count = 0;
	auto asar_prints = asar_getprints(&print_count);
	std::vector<std::string> prints{};
	prints.reserve(print_count);
	for (int i = 0; i < print_count; i++) {
		prints.push_back({ asar_prints[i] });
		trim(prints[i]);
	}
	if (cfg.Debug)
		fmt::print("{}\n", spr.asm_file);
	if (print_count > 2 && cfg.Debug)
		fmt::print("Prints:\n");

	for (const auto& print : prints) {
		if (!print.compare(0, 4, "INIT"))
			ptr_map["init"] = std::stol(print.substr(4), nullptr, 16);
		else if (!print.compare(0, 4, "MAIN"))
			ptr_map["main"] = std::stol(print.substr(4), nullptr, 16);
		else if (!print.compare(0, 4, "CAPE") && spr.sprite_type == 1)
			ptr_map["cape"] = std::stol(print.substr(4), nullptr, 16);
		else if (!print.compare(0, 9, "CARRIABLE") && spr.sprite_type == 0)
			ptr_map["carriable"] = std::stol(print.substr(9), nullptr, 16);
		else if (!print.compare(0, 7, "CARRIED") && spr.sprite_type == 0)
			ptr_map["carried"] = std::stol(print.substr(7), nullptr, 16);
		else if (!print.compare(0, 6, "KICKED") && spr.sprite_type == 0)
			ptr_map["kicked"] = std::stol(print.substr(6), nullptr, 16);
		else if (!print.compare(0, 5, "MOUTH") && spr.sprite_type == 0)
			ptr_map["mouth"] = std::stol(print.substr(5), nullptr, 16);
		else if (!print.compare(0, 4, "GOAL") && spr.sprite_type == 0)
			ptr_map["goal"] = std::stol(print.substr(4), nullptr, 16);
		else if (!print.compare(0, 4, "VERG")) {
			// if the user has put $ to indicate the hex number we skip it
			auto required_version = std::stol(print.substr(print[4] == '$' ? 5 : 4), nullptr, 16);
			if (PixiConfig::VERSION < required_version) {
				ErrorState::pixi_error("The sprite {} requires to be inserted at least with Pixi 1.{:02X}, this is Pixi 1.{:02X}\n",
					spr.asm_file, required_version, PixiConfig::VERSION);
			}
		}
		else if (cfg.Debug) {
			fmt::print("\t{}\n", print);
		}
	}
	spr.table.init = Pointer(ptr_map["init"]);
	spr.table.main = Pointer(ptr_map["main"]);
	if (spr.table.init.is_empty() && spr.table.main.is_empty())
		ErrorState::pixi_error("Sprite {} had neither INIT nor MAIN defined in its file, insertion has been aborted.", spr.asm_file);
	if (spr.sprite_type == 1)
		spr.extended_cape_ptr = Pointer(ptr_map["cape"]);
	else if (spr.sprite_type == 0) {
		spr.ptrs.carried() = Pointer(ptr_map["carried"]);
		spr.ptrs.carriable() = Pointer(ptr_map["carriable"]);
		spr.ptrs.kicked() = Pointer(ptr_map["kicked"]);
		spr.ptrs.mouth() = Pointer(ptr_map["mouth"]);
		spr.ptrs.goal() = Pointer(ptr_map["goal"]);
	}
	if (cfg.Debug) {
		if (spr.sprite_type == 0)
			fmt::print("\tINIT: ${:06X}\n\tMAIN: ${:06X}\n"
				"\tCARRIABLE: ${:06X}\n\tCARRIED: ${:06X}\n\tKICKED: ${:06X}\n"
				"\tMOUTH: ${:06X}\n\tGOAL: ${:06X}"
				"\n__________________________________\n",
				spr.table.init.addr(), spr.table.main.addr(), spr.ptrs.carriable().addr(),
				spr.ptrs.carried().addr(), spr.ptrs.kicked().addr(), spr.ptrs.mouth().addr(), spr.ptrs.goal().addr());
		else if (spr.sprite_type == 1)
			fmt::print("\tINIT: ${:06X}\n\tMAIN: ${:06X}\n\tCAPE: ${:06X}"
				"\n__________________________________\n",
				spr.table.init.addr(), spr.table.main.addr(), spr.extended_cape_ptr.addr());
		else
			fmt::print("\tINIT: ${:06X}\n\tMAIN: ${:06X}\n"
				"\n__________________________________\n",
				spr.table.init.addr(), spr.table.main.addr());
	}
	return retval;
}

void Rom::close()
{
	DEBUGFMTMSG("Writing to ROM, size: {:X} bytes\n", m_data.size());
	FILE* fp = fileopen(m_name.c_str(), "wb");
	size_t written = fwrite(m_data.start(), sizeof(uint8_t), m_data.size(), fp);
	assert(written == m_data.size());
	fclose(fp);
}

void Rom::run_checks()
{
	auto version = at_snes(0x02FFE2 + 4);
	if (version > PixiConfig::VERSION && version != 0xFF) {
		ErrorState::pixi_error("The ROM has been patched with a newer version of PIXI (1.{:02d}) already. \nThis is version 1.{:02d}\nPlease get a newer version.\n", version, PixiConfig::VERSION);
	}

	auto lm_edit_ptr = pointer_at_snes(0x06F624);
	if (lm_edit_ptr == 0xFFFFFF) {
		ErrorState::pixi_warning("You're inserting Pixi without having modified a level in Lunar Magic, this will cause bugs\nDo you "
			"want to abort insertion now [y/n]?\nIf you choose 'n', to fix the bugs just reapply Pixi after having "
			"modified a level\n");
		char c = (char)getchar();
		if (tolower(c) == 'y') {
			ErrorState::pixi_error("Insertion was stopped, press any button to exit...\n");
		}
		fflush(stdin);
	}

	auto vram_jump = at_snes(0x00F6E4);
	if (vram_jump != 0x5C) {
		ErrorState::pixi_error("You haven't installed the VRAM optimization patch in Lunar Magic, this will cause many features of "
			"Pixi to work incorrectly, insertion was aborted...\n");
	}
}
