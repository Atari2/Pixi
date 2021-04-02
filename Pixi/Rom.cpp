#include "Rom.h"

Rom::Rom(std::string romname) : m_name(romname)
{
	m_data.from_file(fileopen(romname.c_str(), "rb"));
	m_header_offset = m_data.size() & 0x7FFF;
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
	debug("Correctly instantiated rom \"{}\" with mapper {}\n", m_name, MapperToString(m_mapper));
}

constexpr MapperType Rom::mapper()
{
	return m_mapper;
}

constexpr size_t Rom::size()
{
	return m_data.size() - m_header_offset;
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

void Rom::close()
{
    FILE* fp = fileopen(m_name.c_str(), "wb");
    fwrite(m_data.start(), sizeof(uint8_t), m_data.size(), fp);
}
