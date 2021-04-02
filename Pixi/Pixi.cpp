// Pixi.cpp : Defines the entry point for the application.
//

#include "Pixi.h"

int main(int argc, char* argv[]) {
	if (!asar_init())
		pixi_error("Asar library is missing or couldn't be initialized, please redownload the tool or the dll.\n");
	PixiConfig cfg{ argc, argv };
	Rom rom{ cfg.RomName };
	fmt::print("{:02X}\n", rom.at_snes(0x00F606));
	Sprite spr{"test.json"};
	spr.parse();
	spr.print(stdout);
	return 0;
}
