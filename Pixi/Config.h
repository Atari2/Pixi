#include <algorithm>
#include <array>
#include <string>
#include "Util.h"

enum class PathType : int {
	Routines,
	Sprites,
	Generators,
	Shooters,
	List,
	Asm,
	Extended,
	Cluster,
	// Overworld,
	SIZE // this is here as a shorthand for counting how many elements are in the enum
};

enum class ExtType : int { Ssc, Mwt, Mw2, S16, SIZE };

struct Paths {
	static constexpr int ArrSize = FromEnum(PathType::SIZE);
	static constexpr std::array<std::string_view, ArrSize> prefixes{ "-l", "-a", "-sp", "-sh", "-g", "-e", "-c", "-r" };
	std::string list{ "list.txt" };
	std::string pasm{ "asm/" };
	std::string sprites{ "sprites/" };
	std::string shooters{ "shooters/" };
	std::string generators{ "generators/" };
	std::string extended{ "extended/" };
	std::string cluster{ "cluster/" };
	std::string routines{ "routines/" };

	inline constexpr std::string& operator[](int index) noexcept {
		std::array<std::string*, ArrSize> paths{ &routines, &sprites, &generators, &shooters,
												 &list,     &pasm,    &extended,   &cluster };
		index = std::clamp(index, 0, (int)paths.size() - 1);
		return *paths[index];
	};

	inline constexpr const std::string& operator[](int index) const noexcept {
		std::array<const std::string*, ArrSize> paths{ &routines, &sprites, &generators, &shooters,
													   &list,     &pasm,    &extended,   &cluster };
		index = std::clamp(index, 0, (int)paths.size() - 1);
		return *paths[index];
	};

	inline constexpr std::string& operator[](PathType index) noexcept {
		std::array<std::string*, ArrSize> paths{ &routines, &sprites, &generators, &shooters,
												 &list,     &pasm,    &extended,   &cluster };
		return *paths[FromEnum(index)];
	};

	inline constexpr const std::string& operator[](PathType index) const noexcept {
		std::array<const std::string*, ArrSize> paths{ &routines, &sprites, &generators, &shooters,
													   &list,     &pasm,    &extended,   &cluster };
		return *paths[FromEnum(index)];
	};
};

struct Extensions {
	static constexpr int ArrSize = FromEnum(ExtType::SIZE);
	static constexpr std::array<std::string_view, ArrSize> prefixes{ "-ssc", "-mwt", "-mw2", "-s16" };
	std::string ssc{};
	std::string mwt{};
	std::string mw2{};
	std::string s16{};

	inline constexpr std::string& operator[](int index) noexcept {
		std::array<std::string*, ArrSize> exts{ &ssc, &mwt, &mw2, &s16 };
		index = std::clamp(index, 0, (int)exts.size() - 1);
		return *exts[index];
	};

	inline constexpr const std::string& operator[](int index) const noexcept {
		std::array<const std::string*, ArrSize> exts{ &ssc, &mwt, &mw2, &s16 };
		index = std::clamp(index, 0, (int)exts.size() - 1);
		return *exts[index];
	};

	inline constexpr std::string& operator[](ExtType index) noexcept {
		std::array<std::string*, ArrSize> exts{ &ssc, &mwt, &mw2, &s16 };
		return *exts[FromEnum(index)];
	};

	inline constexpr const std::string& operator[](ExtType index) const noexcept {
		std::array<const std::string*, ArrSize> exts{ &ssc, &mwt, &mw2, &s16 };
		return *exts[FromEnum(index)];
	};
};

struct PixiConfig {
	static constexpr char TEMP_SPR_FILE[13] = "spr_temp.asm";
	static constexpr int VERSION = 0x32;
	static constexpr int DEFAULT_ROUTINES = 100;
	static constexpr int MAX_ROUTINES = 310;
	inline static const ByteArray<uint8_t, 4> versionflag { VERSION, 0x00, 0x00, 0x00 };
	using Iter = std::vector<std::string>::const_iterator;
	PixiConfig() = default;
	PixiConfig(int argc, char* argv[]) {
		if (argc < 2) {
			wait_before_exit(argc);
			set_rom_name(argc, argv);
		}
		else
			parse_cmd_line_args(argc, argv);
	}
	Paths m_Paths{};
	Extensions m_Extensions{};
	bool KeepFiles = false;
	bool PerLevel = false;
	bool disable255Sprites = false;
	bool ExtMod = true;
	bool DisableMeiMei = false;
	bool Warnings = false;
	bool Debug = false;
	int Routines = 100;
	std::vector<std::string> WarningList{};
	std::string PixiExe{};
	std::string RomName{};
	std::string AsmDir{};
	std::string AsmDirPath{};
#ifdef WIN32
	std::string lm_handle{};
	uint16_t verification_code = 0;
	HWND window_handle = 0;
#endif

	inline void set_rom_name(int argc, char* argv[]) {
		RomName = argc < 2 ? ask("Insert ROM name here: ") : argv[1];
	}

	void parse_cmd_line_args(int argc, char* argv[]);
	void correct_paths();
	bool areConfigFlagsToggled();
	void create_config_file();
	void create_shared_patch();
	std::vector<std::string> list_extra_asm(const char* folder);
	void emit_warnings();
	void fremove(const std::string& dir, const char* name);
	void cleanup();
	bool set_path(Iter& iter, Iter& end, std::string_view pre, PathType type);
	bool set_ext(Iter& iter, Iter& end, std::string_view pre, ExtType type);
	void print_help();
	const std::string& require_next(Iter& iter, Iter& end);

	~PixiConfig() = default;
};