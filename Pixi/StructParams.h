#pragma once
#include <vector>
#include <string_view>
#include <initializer_list>
#include "MemoryFile.h"

template <typename Unit, typename = std::enable_if_t<sizeof(Unit) == 1>>
class StructParams {
	constexpr inline static std::string_view w1005 = "W1005";
	constexpr inline static std::string_view w1001 = "W1001";
	constexpr inline static struct warnsetting setting[2] = {
		{
			w1005.data(),
			false
		},
		{
			w1001.data(),
			false
		}
	};
	struct patchparams* params = nullptr;
	std::vector<MemoryFile<Unit>> files;

	template <typename Elem, typename... Elems>
	void append(Elem&& elem, Elems&&... elems) {
		files.push_back(std::move(elem));
		append(elems...);
	}

	template <typename Elem>
	void append(Elem&& elem) {
		files.push_back(std::move(elem));
	}
	void setup_base(uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		params = new struct patchparams;
		params->structsize = (int)sizeof(patchparams);
		params->romdata = (char*)rom_data;
		params->buflen = max_rom_size;
		params->romlen = &rom_size;

		params->includepaths = nullptr;
		params->numincludepaths = 0;

		params->should_reset = true;

		params->additional_defines = nullptr;
		params->additional_define_count = 0;

		params->stdincludesfile = nullptr;
		params->stddefinesfile = nullptr;

		params->warning_settings = setting;
		params->warning_setting_count = 2;

		params->override_checksum_gen = false;
		params->generate_checksum = true;
	}
public:

	StructParams() = default;

	StructParams<Unit>& operator=(StructParams&& other) noexcept {
		params = other.params;
		other.params = nullptr;
		files = std::move(other.files);
		return *this;
	}

	template <typename... Args>
	StructParams(Args&&... patches) {
		if constexpr (sizeof...(patches) == 0)
			return;
		append(patches...);
	}

	template <size_t S>
	StructParams(std::array<MemoryFile<Unit>, S>& patches) {
		files.reserve(S);
		for (auto&& f : patches)
			files.push_back(std::move(f));
	}

	std::string_view PatchLoc() {
		if (files.size() > 0)
			return files[0].Path();
		return "";
	}

	struct patchparams* construct(const MemoryFile<char>& sprite, uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = sprite.Path().data();
		size_t curr_size = files.size();
		auto memory_files = new memoryfile[curr_size + 1];
		for (size_t i = 0; i < curr_size; i++) {
			memory_files[i].path = files[i].Path().data();
			memory_files[i].buffer = (void*)files[i].Data().data();
			memory_files[i].length = files[i].Data().size();
		}
		memory_files[curr_size].path = sprite.Path().data();
		memory_files[curr_size].buffer = sprite.Data().data();
		memory_files[curr_size].length = sprite.Data().size();
		params->memory_files = memory_files;
		params->memory_file_count = curr_size + 1;
		return params;
	}

	struct patchparams* construct(uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = files[0].Path().data();
		auto memory_files = new memoryfile[files.size()];
		for (size_t i = 0; i < files.size(); i++) {
			memory_files[i].path = files[i].Path().data();
			memory_files[i].buffer = (void*)files[i].Data().data();
			memory_files[i].length = files[i].Data().size();
		}
		params->memory_files = memory_files;
		params->memory_file_count = files.size();
		return params;
	}

	struct patchparams* construct(std::string_view path, uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = path.data();

		auto memory_files = new memoryfile[files.size()];
		for (size_t i = 0; i < files.size(); i++) {
			memory_files[i].path = files[i].Path().data();
			memory_files[i].buffer = (void*)files[i].Data().data();
			memory_files[i].length = files[i].Data().size();
		}
		params->memory_files = memory_files;
		params->memory_file_count = files.size();
		return params;
	}

	~StructParams()
	{
		if (params != nullptr)
			delete[] params->memory_files;
		delete params;
		params = nullptr;
	}
};