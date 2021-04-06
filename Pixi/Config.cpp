#include "Config.h"

const std::string& PixiConfig::require_next(Iter& iter, Iter& end) {
	if (iter + 1 == end) ErrorState::pixi_error("Requiring next parameter for {} failed\n", *iter);
	return *(++iter);
}

bool PixiConfig::set_path(Iter& iter, Iter& end, std::string_view pre, PathType type) {
	if (*iter == pre) {
		m_Paths[type] = require_next(iter, end);
		return true;
	}
	return false;
}

bool PixiConfig::set_ext(Iter& iter, Iter& end, std::string_view pre, ExtType type) {
	if (*iter == pre) {
		m_Extensions[type] = require_next(iter, end);
		return true;
	}
	return false;
}

void PixiConfig::parse_cmd_line_args(int argc, char* argv[])
{
	PixiExe = argv[0];
	RomName = argv[argc - 1];
	std::vector<std::string> vargv{};
	vargv.reserve(argc - 2);
	for (int i = 1; i < argc - 1; i++)
		vargv.push_back(argv[i]);
	Iter end = vargv.cend();

	for (Iter it = vargv.cbegin(); it != end; it++) {
		const std::string& arg = *it;
		if (arg == "-h" || arg == "--help")
			print_help();
		else if (arg == "-d" || arg == "--debug") {
			Debug = true;
		}
		else if (arg == "-k") {
			KeepFiles = true;
		}
		else if (arg == "-nr") {
			Routines = std::clamp(std::atoi(require_next(it, end).c_str()), DEFAULT_ROUTINES, MAX_ROUTINES);
		}
		else if (arg == "-pl") {
			PerLevel = true;
		}
		else if (arg == "-npl") {
			PerLevel = false;
		}
		else if (arg == "-d255spl") {
			disable255Sprites = true;
		}
		else if (arg == "-w") {
			Warnings = true;
		}
		else if (arg == "-ext-off") {
			ExtMod = false;
		}
		// TODO: handle meimei command arguments
		else if (arg == "-meimei-a") {

		}
		else if (arg == "-meimei-d") {

		}
		else if (arg == "-meimei-k") {

		}
		else if (arg == "-meimei-off") {
			DisableMeiMei = true;
		}
#ifdef WIN32
		else if (arg == "-lm-handle") {
			lm_handle = require_next(it, end);
			window_handle = (HWND)std::stoull(lm_handle, 0, 16);
			verification_code = (uint16_t)(std::stoi(lm_handle.substr(lm_handle.find_first_of(':') + 1), 0, 16));
		}
#endif
		else {
			bool set = false;
			for (int i = 0; i < Paths::ArrSize; i++) {
				if (set_path(it, end, Paths::prefixes[i], (PathType)i)) {
					set = true;
					break;
				}
			}
			if (set) continue;
			for (int i = 0; i < Extensions::ArrSize; i++) {
				if (set_ext(it, end, Extensions::prefixes[i], (ExtType)i)) {
					set = true;
					break;
				}
			}
			if (set) continue;
			ErrorState::pixi_error("Invalid command line option \"{}\"\n", arg);
		}
	}

}

void PixiConfig::correct_paths() {
	for (int i = 0; i < FromEnum(PathType::SIZE); i++) {
		if (i == FromEnum(PathType::List))
			set_paths_relative_to(m_Paths[i], RomName);
		else
			set_paths_relative_to(m_Paths[i], PixiExe);
		ErrorState::debug("Paths[{}] = {}\n", i, m_Paths[i]);
	}
	AsmDir = m_Paths[FromEnum(PathType::Asm)];
	AsmDirPath = cleanPathTrail(AsmDir);

	for (int i = 0; i < FromEnum(ExtType::SIZE); i++) {
		set_paths_relative_to(m_Extensions[i], RomName);
		ErrorState::debug("Extensions[{}] = {}\n", i, m_Extensions[i]);
	}
}

bool PixiConfig::areConfigFlagsToggled() {
	return PerLevel || disable255Sprites || true;
}

void PixiConfig::create_config_file()
{
	std::string path = AsmDirPath + "/config.asm";
	if (areConfigFlagsToggled()) {
		FILE* config = fileopen(path.c_str(), "w");
		fmt::print(config, "!PerLevel = {:d}\n", (int)PerLevel);
		fmt::print(config, "!Disable255SpritesPerLevel = {:d}", (int)disable255Sprites);
		fclose(config);
	}
}

void PixiConfig::create_shared_patch()
{
	const std::string& routinepath = m_Paths[PathType::Routines];
	std::string escapedRoutinepath = escapeDefines(routinepath, R"(\\\!)");
	FILE* shared_patch = fileopen("shared.asm", "w");
	fmt::print(shared_patch, "macro include_once(target, base, offset)\n"
		"	if !<base> != 1\n"
		"		!<base> = 1\n"
		"		pushpc\n"
		"		if read3(<offset>+$03E05C) != $FFFFFF\n"
		"			<base> = read3(<offset>+$03E05C)\n"
		"		else\n"
		"			freecode cleaned\n"
		"				global #<base>:\n"
		"				print \"    Routine: <base> inserted at $\",pc\n"
		"				namespace <base>\n"
		"				incsrc \"<target>\"\n"
		"               namespace off\n"
		"			ORG <offset>+$03E05C\n"
		"				dl <base>\n"
		"		endif\n"
		"		pullpc\n"
		"	endif\n"
		"endmacro\n");
	int routine_count = 0;
	if (!std::filesystem::exists(cleanPathTrail(m_Paths[PathType::Routines]))) {
		ErrorState::pixi_error("Couldn't open folder \"{}\" for reading.", routinepath);
	}
	try {
		for (const auto& routine_file : std::filesystem::directory_iterator(routinepath)) {
			std::string name(routine_file.path().filename().generic_string());
			if (routine_count > DEFAULT_ROUTINES) {
				ErrorState::pixi_error("More than 100 routines located. Please remove some. \n");
			}
			if (nameEndWithAsmExtension(name)) {
				name = name.substr(0, name.length() - 4);
				fmt::print(shared_patch,
					"!{} = 0\n"
					"macro {}()\n"
					"\t%include_once(\"{}{}.asm\", {}, ${:02X})\n"
					"\tJSL {}\n"
					"endmacro\n",
					name, name, escapedRoutinepath, name, name, routine_count * 3,
					name);
				routine_count++;
			}
		}
	}
	catch (const std::filesystem::filesystem_error& err) {
		ErrorState::pixi_error("Trying to read folder \"{}\" returned \"{}\", aborting insertion\n", routinepath, err.what());
	}
	fmt::print("{} Shared routines registered in \"{}\"\n", routine_count, routinepath);
	fclose(shared_patch);
}

std::vector<std::string> PixiConfig::list_extra_asm(const char* folder) {
	std::string path = AsmDirPath + folder;
	std::vector<std::string> extraAsm{};
	if (!std::filesystem::exists(cleanPathTrail(path))) {
		return extraAsm;
	}

	try {
		for (auto& file : std::filesystem::directory_iterator(path)) {
			std::string spath = file.path().generic_string();
			if (nameEndWithAsmExtension(spath))
				extraAsm.push_back(spath);
		}
	}
	catch (const std::filesystem::filesystem_error& err) {
		ErrorState::pixi_error("Trying to read folder \"{}\" returned \"{}\", aborting insertion\n", path, err.what());
	}
	if (extraAsm.size() > 0)
		std::sort(extraAsm.begin(), extraAsm.end());
	return extraAsm;
}

void PixiConfig::emit_warnings() {
	if (!WarningList.empty() && Warnings) {
		fmt::print("One or more warnings have been detected:\n");
		for (const std::string& warning : WarningList) {
			ErrorState::pixi_warning("{}\n", warning);
		}
		ErrorState::pixi_warning("Do you want to continue insertion anyway? [Y/n] (Default is yes):\n");
		char c = (char)getchar();
		if (tolower(c) == 'y') {
			ErrorState::pixi_error("Insertion was stopped, press any button to exit...\n");
		}
		fflush(stdin);
	}
}

void PixiConfig::fremove(const std::string& dir, const char* name) {
	std::string fullpath = dir + name;
	remove(fullpath.c_str());
}

void PixiConfig::cleanup()
{
	constexpr int ASM = FromEnum(PathType::Asm);
	if (!KeepFiles) {
		fremove(m_Paths[ASM], "_versionflag.bin");

		fremove(m_Paths[ASM], "_DefaultTables.bin");
		fremove(m_Paths[ASM], "_CustomStatusPtr.bin");
		if (PerLevel) {
			fremove(m_Paths[ASM], "_PerLevelLvlPtrs.bin");
			fremove(m_Paths[ASM], "_PerLevelSprPtrs.bin");
			fremove(m_Paths[ASM], "_PerLevelT.bin");
			fremove(m_Paths[ASM], "_PerLevelCustomPtrTable.bin");
		}

		fremove(m_Paths[ASM], "_ClusterPtr.bin");
		fremove(m_Paths[ASM], "_ExtendedPtr.bin");
		fremove(m_Paths[ASM], "_ExtendedCapePtr.bin");
		// remove("asm/_OverworldMainPtr.bin");
		// remove("asm/_OverworldInitPtr.bin");

		fremove(m_Paths[ASM], "_CustomSize.bin");
		remove("shared.asm");
		remove(TEMP_SPR_FILE);

		fremove(m_Paths[ASM], "_cleanup.asm");
	}
}

void PixiConfig::print_help()
{
	fmt::print("Version 1.{:02}\n", VERSION);
	fmt::print("Usage: pixi <options> <ROM>\nOptions are:\n");
	fmt::print("-d\t\tEnable debug output, the following flag <-out> only works when this is set\n");
	fmt::print("-k\t\tKeep debug files\n");
	fmt::print("-l  <listpath>\tSpecify a custom list file (Default: {})\n",
		m_Paths[FromEnum(PathType::List)]);
	fmt::print("-pl\t\tPer level sprites - will insert perlevel sprite code\n");
	fmt::print("-npl\t\tSame as the current default, no sprite per level will be inserted, left dangling for "
		"compatibility reasons\n");
	fmt::print("-d255spl\t\tDisable 255 sprite per level support (won't do the 1938 remap)\n");
	fmt::print("-w\t\tEnable asar warnings check, recommended to use when developing sprites.\n");
	fmt::print("\n");

	fmt::print("-a  <asm>\tSpecify a custom asm directory (Default {})\n",
		m_Paths[FromEnum(PathType::Asm)]);
	fmt::print("-sp <sprites>\tSpecify a custom sprites directory (Default {})\n",
		m_Paths[FromEnum(PathType::Sprites)]);
	fmt::print("-sh <shooters>\tSpecify a custom shooters directory (Default {})\n",
		m_Paths[FromEnum(PathType::Shooters)]);
	fmt::print("-g  <generators>\tSpecify a custom generators directory (Default {})\n",
		m_Paths[FromEnum(PathType::Generators)]);
	fmt::print("-e  <extended>\tSpecify a custom extended sprites directory (Default {})\n",
		m_Paths[FromEnum(PathType::Extended)]);
	fmt::print("-c  <cluster>\tSpecify a custom cluster sprites directory (Default {})\n",
		m_Paths[FromEnum(PathType::Cluster)]);
	fmt::print("\n");

	fmt::print("-r   <routines>\tSpecify a shared routine directory (Default {})\n",
		m_Paths[FromEnum(PathType::Routines)]);
	fmt::print("-nr <number>\tSpecify limit to shared routines (Default %d, Maximum value %d)\n", DEFAULT_ROUTINES,
		MAX_ROUTINES);
	fmt::print("\n");

	fmt::print("-ext-off\t Disables extmod file logging (check LM's readme for more info on what extmod is)\n");
	fmt::print("-ssc <append ssc>\tSpecify ssc file to be copied into <romname>.ssc\n");
	fmt::print("-mwt <append mwt>\tSpecify mwt file to be copied into <romname>.mwt\n");
	fmt::print("-mw2 <append mw2>\tSpecify mw2 file to be copied into <romname>.mw2, the provided file is assumed "
		"to have 0x00 first byte sprite header and the 0xFF end byte\n");
	fmt::print("-s16 <base s16>\tSpecify s16 file to be used as a base for <romname>.s16\n");
	fmt::print("     Do not use <romname>.xxx as an argument as the file will be overwriten\n");

#ifdef WIN32
	fmt::print("-lm-handle <lm_handle_code>\t To be used only within LM's custom user toolbar file, it receives "
		"LM's handle to reload the rom\n");
#endif

	fmt::print("\nMeiMei flags:\n");
	fmt::print("-meimei-off\t\tShuts down MeiMei completely\n");
	fmt::print("-meimei-a\t\tEnables always remap sprite data\n");
	fmt::print("-meimei-k\t\tEnables keep temp patches files\n");
	fmt::print("-meimei-d\t\tEnables debug for MeiMei patches\n\n");
	exit(0);
}
