#include "Config.h"

const std::string& PixiConfig::require_next(Iter& iter, Iter& end) {
	if (iter + 1 == end) pixi_error("Requiring next parameter for {} failed\n", *iter);
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
			m_Debug.isOn = true;
			m_Debug.output = stdout;
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
			pixi_error("Invalid command line option \"{}\"\n", arg);
		}
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
