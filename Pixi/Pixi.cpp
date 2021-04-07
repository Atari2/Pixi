// Pixi.cpp : Defines the entry point for the application.
//

#include "Pixi.h"

int main(int argc, char* argv[]) {
	if (!ErrorState::asar_init_wrap())
		ErrorState::pixi_error("Asar library is missing or couldn't be initialized, please redownload the tool or the dll.\n");
	PixiConfig cfg{ argc, argv };
	MeiMei::setCfg(cfg.m_meimei);
	Rom rom{ cfg.RomName };
	rom.run_checks();
	if (!cfg.DisableMeiMei) {
		MeiMei::initialize(cfg.RomName);
	}
	cfg.correct_paths();
	cfg.create_config_file();
	auto extraDefines = cfg.list_extra_asm("/ExtraDefines");
	SpritesData sprdata{ rom, cfg };
	sprdata.populate(cfg);
	rom.clean(cfg);
	cfg.create_shared_patch();
	sprdata.patch_sprites_wrap(extraDefines, cfg);
	cfg.emit_warnings();
	ErrorState::debug("Sprites successfully patched.\n");
	sprdata.serialize(cfg);
	rom.patch(cfg.m_Paths[PathType::Asm], "main.asm", cfg);
	rom.patch(cfg.m_Paths[PathType::Asm], "cluster.asm", cfg);
	rom.patch(cfg.m_Paths[PathType::Asm], "extended.asm", cfg);
	auto extraHijacks = cfg.list_extra_asm("/ExtraHijacks");
	if (extraHijacks.size() > 0 && cfg.Debug) {
		fmt::print("-------- ExtraHijacks prints --------\n");
	}
	std::for_each(extraHijacks.cbegin(), extraHijacks.cend(), [&rom, &cfg](const std::string& patch) mutable {
		int extra_print_count = 0;
		rom.patch(patch, cfg);
		if (cfg.Debug) {
			auto prints = asar_getprints(&extra_print_count);
			for (int i = 0; i < extra_print_count; i++)
				fmt::print("\tFrom file \"{}\": {}\n", patch, prints[i]);
		}
		});
	fmt::print("\nAll sprites applied successfully\n");
	if (cfg.ExtMod)
		cfg.create_lm_restore();
	cfg.cleanup();
	rom.close();
	ErrorState::asar_close_wrap();
	int retval = 0;
	if (!cfg.DisableMeiMei) {
		retval = MeiMei::run(cfg);
	}
#ifdef WIN32
	if (!cfg.lm_handle.empty()) {
		uint32_t IParam = (cfg.verification_code << 16) + 2;
		PostMessage(cfg.window_handle, 0xBECB, 0, IParam);
	}
#endif
	return retval;
}
