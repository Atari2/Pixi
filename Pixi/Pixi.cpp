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
	fmt::print("{:02X}\n", rom.at_snes(0x00F606));
	Sprite spr{"test.json"};
	spr.parse();
	spr.print(stdout);
	return 0;
}
