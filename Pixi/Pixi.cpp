// Pixi.cpp : Defines the entry point for the application.
//

#include "Pixi.h"

int main(int argc, char* argv[]) {
	if (!ErrorState::asar_init_wrap())
		ErrorState::pixi_error("Asar library is missing or couldn't be initialized, please redownload the tool or the dll.\n");
	PixiConfig cfg{ argc, argv };
	Rom rom{ cfg.RomName };
	rom.run_checks();
	if (!cfg.DisableMeiMei) {
		// TODO: meimei initialize
	}
	cfg.correct_paths();
	cfg.create_config_file();
	auto extraDefines = cfg.list_extra_asm("/ExtraDefines");
	SpritesData sprdata{rom, cfg};
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
	// TODO: apply hijacks
	cfg.cleanup();
	rom.close();
	asar_close();
	int retval = 0;
	if (!cfg.DisableMeiMei) {
		// TODO: implement meimei
	}
#ifdef WIN32
	if (!cfg.lm_handle.empty()) {
		uint32_t IParam = (cfg.verification_code << 16) + 2;
		PostMessage(cfg.window_handle, 0xBECB, 0, IParam);
	}
#endif
	return retval;
}
