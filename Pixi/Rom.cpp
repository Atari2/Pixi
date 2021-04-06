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
	ErrorState::debug("Correctly instantiated rom \"{}\" with mapper {}\n", m_name, MapperToString(m_mapper));
}

constexpr MapperType Rom::mapper()
{
	return m_mapper;
}

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

uint8_t Rom::at_snes(size_t address)
{
	return m_data[snes_to_pc(address)];
}

int Rom::pointer_at_snes(size_t address) {
	auto offset = snes_to_pc(address);
	return m_data[offset] << 16 | m_data[offset + 1] << 8 | m_data[offset + 2];
}

uint8_t Rom::at(size_t offset)
{
	return m_data[offset];
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

void Rom::clean(const PixiConfig& cfg)
{
	// TODO : implement clean
}

bool Rom::patch(const std::string& dir, const std::string& file, PixiConfig& cfg) {
	std::string path = dir + file;
	return patch(path, cfg);
}

bool Rom::patch(const std::string& file, PixiConfig& cfg) {
	std::string patch_path = std::filesystem::absolute(file).generic_string();
	if (!asar_patch(patch_path.c_str(), (char*)m_data.ptr_at(m_header_offset), MAX_ROM_SIZE, &size())) {
		ErrorState::debug("Failure. Try fetch errors:\n");
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
	ErrorState::debug("Patching for {} successful\n", file);
	return true;
}

void Rom::close()
{
	FILE* fp = fileopen(m_name.c_str(), "wb");
	fwrite(m_data.start(), sizeof(uint8_t), m_data.size(), fp);
}

void Rom::run_checks()
{
	auto version = at_snes(0x02FFE2 + 4);
	if (version > PixiConfig::VERSION && version != 0xFF) {
		ErrorState::pixi_error("The ROM has been patched with a newer version of PIXI (1.{02d}) already. \nThis is version 1.{:02d}\nPlease get a newer version.\n", version, PixiConfig::VERSION);
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
