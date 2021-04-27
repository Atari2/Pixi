#pragma once
#include <vector>
#include <string_view>
#include <initializer_list>
#include "MemoryFile.h"

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
	size_t n_files;
	std::vector<std::pair<const void*, size_t>> file_data;
	std::vector<std::string_view> file_paths;

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

	template <typename File, typename... Files>
	void append(File& file, Files&... files) {
		file_data.push_back(std::make_pair(file.Data(), file.Size()));
		file_paths.push_back(file.Path());
		append(files...);
	}

	template <typename File>
	void append(File& file) {
		file_data.push_back(std::make_pair(file.Data(), file.Size()));
		file_paths.push_back(file.Path());
	}
public:

	StructParams() = default;

	StructParams& operator=(StructParams&& other) = delete;

	template <typename... Files>
	StructParams(Files&... files) {
		if constexpr (sizeof...(files) == 0)
			return;
		n_files = sizeof...(files);
		file_data.reserve(n_files);
		file_paths.reserve(n_files);
		append(files...);
	}

	std::string_view PatchLoc() {
		if (n_files > 0)
			return file_paths[0];
		return "";
	}

	struct patchparams* construct(const MemoryFile& sprite, uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = sprite.Path().data();
		auto memory_files = new memoryfile[n_files + 1];
		for (size_t i = 0; i < n_files; i++) {
			memory_files[i].path = file_paths[i].data();
			memory_files[i].buffer = file_data[i].first;
			memory_files[i].length = file_data[i].second;
		}
		memory_files[n_files].path = sprite.Path().data();
		memory_files[n_files].buffer = sprite.Data();
		memory_files[n_files].length = sprite.Size();
		params->memory_files = memory_files;
		params->memory_file_count = n_files + 1;
		return params;
	}

	struct patchparams* construct(uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = file_paths[0].data();
		auto memory_files = new memoryfile[n_files];
		for (size_t i = 0; i < n_files; i++) {
			memory_files[i].path = file_paths[i].data();
			memory_files[i].buffer = file_data[i].first;
			memory_files[i].length = file_data[i].second;
		}
		params->memory_files = memory_files;
		params->memory_file_count = n_files;
		return params;
	}

	struct patchparams* construct(std::string_view path, uint8_t* rom_data, size_t max_rom_size, int& rom_size) {
		setup_base(rom_data, max_rom_size, rom_size);
		params->patchloc = path.data();

		auto memory_files = new memoryfile[n_files];
		for (size_t i = 0; i < n_files; i++) {
			memory_files[i].path = file_paths[i].data();
			memory_files[i].buffer = file_data[i].first;
			memory_files[i].length = file_data[i].second;
		}
		params->memory_files = memory_files;
		params->memory_file_count = n_files;
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