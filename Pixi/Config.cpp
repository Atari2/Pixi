#include "Config.h"

PixiConfig::PixiConfig(int argc, char* argv[]) {

	if (argc < 2) {
		PixiExe = argv[0];
		// if this was started in command line, path starts with ./ usually
		if (PixiExe[0] == '.' && PixiExe[1] == '/')
			PixiExe = PixiExe.substr(2);
		parse_noargs_config_file();
		set_rom_name(argc, argv);
	}
	else {
		auto [args, overwrite] = transform_args(argc, argv);
		parse_config_file(overwrite);
		parse_cmd_line_args(args);
	}
	GlobalKeepFlag = KeepFiles;
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4866 )
#endif
void PixiConfig::parse_noargs_config_file() {
	if (std::filesystem::exists("pixi_conf.toml")) {
		try {
			auto config_table = toml::parse_file("pixi_conf.toml");
			auto paths_ptr = config_table["paths"].as_table();
			if (paths_ptr != nullptr) {
				auto& paths = *paths_ptr;
				m_Paths[PathType::Routines] = paths["routines"].value_or(m_Paths[PathType::Routines]);
				m_Paths[PathType::Sprites] = paths["sprites"].value_or(m_Paths[PathType::Sprites]);
				m_Paths[PathType::Generators] = paths["generators"].value_or(m_Paths[PathType::Generators]);
				m_Paths[PathType::Shooters] = paths["shooters"].value_or(m_Paths[PathType::Shooters]);
				m_Paths[PathType::List] = paths["list"].value_or(m_Paths[PathType::List]);
				m_Paths[PathType::Asm] = paths["asm"].value_or(m_Paths[PathType::Asm]);
				m_Paths[PathType::Extended] = paths["extended"].value_or(m_Paths[PathType::Extended]);
				m_Paths[PathType::Cluster] = paths["cluster"].value_or(m_Paths[PathType::Cluster]);
			}
			auto extensions_ptr = config_table["extensions"].as_table();
			if (extensions_ptr != nullptr) {
				auto& extensions = *extensions_ptr;
				m_Extensions[ExtType::Ssc] = extensions["ssc"].value_or(m_Extensions[ExtType::Ssc]);
				m_Extensions[ExtType::Mwt] = extensions["mwt"].value_or(m_Extensions[ExtType::Mwt]);
				m_Extensions[ExtType::Mw2] = extensions["mw2"].value_or(m_Extensions[ExtType::Mw2]);
				m_Extensions[ExtType::S16] = extensions["s16"].value_or(m_Extensions[ExtType::S16]);
			}
			auto meimei_ptr = config_table["meimei"].as_table();
			if (meimei_ptr != nullptr) {
				auto& meimei = *meimei_ptr;
				m_meimei.always = meimei["alwaysremap"].value_or(false);
				m_meimei.debug = meimei["debuginfo"].value_or(false);
				m_meimei.keep = meimei["keeptemp"].value_or(false);
			}
			KeepFiles = config_table["keeptemp"].value_or(false);
			PerLevel = config_table["perlevel"].value_or(false);
			disable255Sprites= config_table["disable255sprites"].value_or(false);
			ExtMod = config_table["extmod"].value_or(true);
			DisableMeiMei = config_table["disablemeimei"].value_or(false);
			Warnings = config_table["warnings"].value_or(false);
			Debug = config_table["debug"].value_or(false);
			Routines = config_table["routines"].value_or(100);
		}
		catch (const toml::parse_error& err) {
			ErrorState::pixi_error("Couldn't parse pixi_conf.toml correctly, error was {}", err.description());
		}
	}
	else {
		std::ofstream outfile{ "pixi_conf.toml" };
		if (!outfile)
			ErrorState::pixi_error("Couldn't read pixi_conf.toml, please check file permissions");
		auto config_table = toml::table{{
			{"paths", toml::table{{
				{"routines", m_Paths[PathType::Routines]},
				{"sprites", m_Paths[PathType::Sprites]},
				{"generators", m_Paths[PathType::Generators]},
				{"shooters", m_Paths[PathType::Shooters]},
				{"list", m_Paths[PathType::List]},
				{"asm", m_Paths[PathType::Asm]},
				{"extended", m_Paths[PathType::Extended]},
				{"cluster", m_Paths[PathType::Cluster]}
				}}},
			{"extensions", toml::table{{
				{"ssc", m_Extensions[ExtType::Ssc]},
				{"mwt", m_Extensions[ExtType::Mwt]},
				{"mw2", m_Extensions[ExtType::Mw2]},
				{"s16", m_Extensions[ExtType::S16]}
				}}},
			{"meimei", toml::table{{
				{"alwaysremap", false},
				{"debuginfo", false},
				{"keeptemp", false}
				}}},
			{"keeptemp", false},
			{"perlevel", false},
			{"disable255sprites", false},
			{"extmod", true},
			{"disablemeimei", false},
			{"warnings", false},
			{"debug", false},
			{"routines", 100}
		} };
		outfile << config_table;
		outfile.flush();
	}
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

void PixiConfig::parse_config_file(bool overwrite) {
	// if the user specified -no-config, we don't care about the config file at all
	if (overwrite)
		return;
	parse_noargs_config_file();
}

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

std::pair<std::vector<std::string>, bool> PixiConfig::transform_args(int argc, char* argv[]) {
	bool overwrite = false;
	PixiExe = argv[0];
	RomName = argv[argc - 1];
	if (PixiExe[0] == '.' && PixiExe[1] == '/')
		PixiExe = PixiExe.substr(2);
	std::vector<std::string> vargv{};
	vargv.reserve(argc - 2);
	for (int i = 1; i < argc - 1; i++)
		vargv.push_back(argv[i]);
	Iter end = vargv.cend();

	auto res = std::find_if(vargv.cbegin(), vargv.cend(), [](const std::string& par) {
		return par == "-no-config";
		});
	if (res != end) {
		vargv.erase(res);
		overwrite = true;
	}
	return std::make_pair(vargv, overwrite);
}

void PixiConfig::parse_cmd_line_args(const std::vector<std::string>& vargv)
{
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
		else if (arg == "-meimei-a") {
			m_meimei.always = true;
		}
		else if (arg == "-meimei-d") {
			m_meimei.debug = true;
		}
		else if (arg == "-meimei-k") {
			m_meimei.keep = true;
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
	DEBUGFMTMSG("{}\n", PixiExe)
	for (int i = 0; i < FromEnum(PathType::SIZE); i++) {
		if (i == FromEnum(PathType::List))
			set_paths_relative_to(m_Paths[i], RomName);
		else
			set_paths_relative_to(m_Paths[i], PixiExe);
		DEBUGFMTMSG("Paths[{}] = {}\n", i, m_Paths[i]);
	}
	AsmDir = m_Paths[FromEnum(PathType::Asm)];
	AsmDirPath = cleanPathTrail(AsmDir);

	for (int i = 0; i < FromEnum(ExtType::SIZE); i++) {
		set_paths_relative_to(m_Extensions[i], RomName);
		DEBUGFMTMSG("Extensions[{}] = {}\n", i, m_Extensions[i]);
	}
}

bool PixiConfig::areConfigFlagsToggled() {
	return PerLevel || disable255Sprites || true;
}

MemoryFile<char> PixiConfig::create_config_file()
{
	MemoryFile<char> file{ AsmDirPath + "/config.asm" };
	if (areConfigFlagsToggled()) {
		file.insertData("!PerLevel = {:d}\n", (int)PerLevel);
		file.insertData("!Disable255SpritesPerLevel = {:d}", (int)disable255Sprites);
	}
	return file;
}

MemoryFile<char> PixiConfig::create_shared_patch()
{
	const std::string& routinepath = m_Paths[PathType::Routines];
	std::string escapedRoutinepath = escapeDefines(routinepath, R"(\\\!)");
	MemoryFile<char> shared_patch{ "shared.asm" };
	shared_patch.insertData("macro include_once(target, base, offset)\n"
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
				shared_patch.insertData(
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
	return shared_patch;
}

void PixiConfig::create_lm_restore()
{
	auto to_write = fmt::format("Pixi v1.{:02X}\t", VERSION);
	auto restorename = RomName.substr(0, RomName.find_last_of('.')) + ".extmod";
	FILE* res = fileopen(restorename.c_str(), "a+");
	size_t size = filesize(res);
	char* contents = new char[size + 1];
	size_t read_size = fread(contents, 1, size, res);
	if (size != read_size)
		ErrorState::pixi_error("Couldn\'t fully read file {}, please check file permissions", restorename);
	contents[size] = '\0';
	if (!ends_with(contents, to_write.c_str())) {
		fseek(res, 0, SEEK_END);
		fmt::print(res, "{}", to_write);
	}
	fclose(res);
	delete[] contents;
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

void PixiConfig::print_help()
{
	fmt::print("Version 1.{:02}\n", VERSION);
	fmt::print("Usage: pixi <options> <ROM>\nOptions are:\n");
	fmt::print("-d\t\tEnable debug output\n");
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
	fmt::print("-no-config\t Disables the use of the config file for this run\n");

	fmt::print("\nMeiMei flags:\n");
	fmt::print("-meimei-off\t\tShuts down MeiMei completely\n");
	fmt::print("-meimei-a\t\tEnables always remap sprite data\n");
	fmt::print("-meimei-k\t\tEnables keep temp patches files\n");
	fmt::print("-meimei-d\t\tEnables debug for MeiMei patches\n\n");
	exit(0);
}
