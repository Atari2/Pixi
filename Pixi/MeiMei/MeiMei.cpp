#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>
#include "MeiMei.h"


void MeiMei::configureSa1Def(const std::string& pathToSa1Def) {
    sa1DefPath = escapeDefines(pathToSa1Def);
}

bool MeiMei::patch(MemoryFile& patch, Rom& rom, PixiConfig& cfg, MemoryFile& binfile) {
    rom.patch(patch, cfg, binfile);
    if (debug) {
        int print_count = 0;
        const char* const* prints = asar_getprints(&print_count);
        for (int i = 0; i < print_count; ++i) {
            fmt::print("\t{}\n", prints[i]);
        }
    }
    return true;
}

MeiMei::MeiMei(const MeiMeiConfig& cfg, const std::string& rom_name) : 
    name(rom_name),
    prev(name),
    always(cfg.always),
    debug(cfg.debug),
    keepTemp(cfg.keep)
{
    prevEx.fill(0x00);
    nowEx.fill(0x00);
    if (prev.read_byte(0x07730F) == 0x42) {
        int addr = prev.snes_to_pc(prev.read_long(0x07730C), false);
        prevEx.write(prev.data().ptr_at(addr), prevEx.size());
    }
}


int MeiMei::validate(bool revert) {
    if (revert) return 1;
    return 0;
}

bool MeiMei::overSize(int size) {
    if (size > SPR_ADDR_LIMIT) {
        fmt::print("Sprite data is too large, size was {:X} when max size is {:X}", size, SPR_ADDR_LIMIT);
        return true;
    }
    return false;
}

int MeiMei::run(PixiConfig& cfg) {
    Rom rom(name);
    if (!ErrorState::asar_init_wrap()) {
        ErrorState::pixi_error("Error: Asar library is missing or couldn't be initialized, please redownload the tool or add the dll.\n");
    }

    int returnValue = run(rom, cfg);

    if (returnValue) {
        prev.close();
        ErrorState::asar_close_wrap();
        fmt::print("\n\nError occurred in MeiMei.\nYour rom has reverted to before pixi insert.\n");
        return returnValue;
    }

    rom.close();
    ErrorState::asar_close_wrap();
    return returnValue;
}

int MeiMei::run(Rom& rom, PixiConfig& cfg) {
    Rom now(name);
    if (prev.read_byte(0x07730F) == 0x42) {
        int addr = now.snes_to_pc(now.read_long(0x07730C), false);
        nowEx.write(now.data().ptr_at(addr), 0x400);
    }

    bool changeEx = false;
    for (int i = 0; i < 0x400; i++) {
        if (prevEx[i] != nowEx[i]) {
            DEBUGFMTMSG("Extra byte counts changed: 0x{:X} {:X} {:X}\n", i, prevEx[i], nowEx[i]);
            changeEx = true;
            break;
        }
    }

    bool revert = changeEx || always;
    if (changeEx) {
        printf("\nExtra bytes change detected\n");
    }

    if (revert) {
        ByteArray<uint8_t, SPR_ADDR_LIMIT> sprAllData{};
        sprAllData.fill(0x00);
        ByteArray<uint8_t, 3> sprCommonData{};
        sprCommonData.fill(0x00);
        bool remapped[0x0200];
        for (int i = 0; i < 0x0200; i++) {
            remapped[i] = false;
        }

        for (int lv = 0; lv < 0x200; lv++) {
            if (remapped[lv])
                continue;

            int sprAddrSNES = (now.read_byte(0x077100 + lv) << 16) + now.read_word(0x02EC00 + lv * 2);
            int sprAddrPC = now.snes_to_pc(sprAddrSNES, false);
            if (sprAddrPC == -1) {
                fmt::print("Sprite Data has invalid address. Address: ${:06X}\n", sprAddrSNES);
                return validate(revert);
            }

            for (int i = 0; i < SPR_ADDR_LIMIT; i++) {
                sprAllData[i] = 0;
            }

            sprAllData[0] = now.read_byte(sprAddrPC);
            int prevOfs = 1;
            int nowOfs = 1;
            bool exlevelFlag = sprAllData[0] & (uint8_t)0x20;
            bool changeData = false;

            while (true) {
                sprCommonData.write(now.data().ptr_at(sprAddrPC + prevOfs), 3);
                if (nowOfs >= SPR_ADDR_LIMIT - 3) {
                    fmt::print("Sprite data is too large! Size is {:X}", nowOfs);
                    return validate(revert);
                }

                if (sprCommonData[0] == 0xFF) {
                    sprAllData[nowOfs++] = 0xFF;
                    if (!exlevelFlag) {
                        break;
                    }

                    sprAllData[nowOfs++] = sprCommonData[1];
                    if (sprCommonData[1] == 0xFE) {
                        break;
                    }
                    else {
                        prevOfs += 2;
                        sprCommonData.write(now.data().ptr_at(sprAddrPC + prevOfs), 3);
                    }
                }

                sprAllData[nowOfs++] = sprCommonData[0]; // YYYYEEsy
                sprAllData[nowOfs++] = sprCommonData[1]; // XXXXSSSS
                sprAllData[nowOfs++] = sprCommonData[2]; // NNNNNNNN

                int sprNum = ((sprCommonData[0] & 0x0C) << 6) | (sprCommonData[2]);

                if (nowEx[sprNum] > prevEx[sprNum]) {
                    changeData = true;
                    int i;
                    for (i = 3; i < prevEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = now.read_byte(sprAddrPC + prevOfs + i);
                        if (overSize(nowOfs)) return validate(revert);
                    }
                    for (; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = 0x00;
                        if (overSize(nowOfs)) return validate(revert);
                    }
                }
                else if (nowEx[sprNum] < prevEx[sprNum]) {
                    changeData = true;
                    for (int i = 3; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = now.read_byte(sprAddrPC + prevOfs + i);
                        if (overSize(nowOfs)) return validate(revert);
                    }
                }
                else {
                    for (int i = 3; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = now.read_byte(sprAddrPC + prevOfs + i);
                        if (overSize(nowOfs)) return validate(revert);
                    }
                }
                prevOfs += prevEx[sprNum];
            }

            prevOfs++;
            if (changeData) {
                // create sprite data binary
                MemoryFile binFile{ fmt::format("_tmp_bin_{:X}.bin", lv), keepTemp };
                MemoryFile spriteDataPatch{ fmt::format("_tmp_{:X}.asm", lv), keepTemp };
                binFile.insertBytes(sprAllData.start(), sprAllData.size());

                // create patch for sprite data binary
                std::string binaryLabel = fmt::format("SpriteData{:X}", lv);
                std::string levelBankAddress = fmt::format("{:06X}", now.pc_to_snes(0x077100 + lv, false));
                std::string levelWordAddress = fmt::format("{:06X}", now.pc_to_snes(0x02EC00 + lv * 2, false));

                // create actual asar patch
                spriteDataPatch.insertString("incsrc \"{}\"\n\n", sa1DefPath);
                spriteDataPatch.insertString("!oldDataPointer = read2(${})|(read1(${})<<16)\n", levelWordAddress, levelBankAddress);
                spriteDataPatch.insertString("!oldDataSize = read2(pctosnes(snestopc(!oldDataPointer)-4))+1\n");
                spriteDataPatch.insertString("autoclean !oldDataPointer\n\n");
                spriteDataPatch.insertString("org ${}\n", levelBankAddress);
                spriteDataPatch.insertString("\tdb {}>>16\n\n", binaryLabel);
                spriteDataPatch.insertString("org ${}\n", levelWordAddress);
                spriteDataPatch.insertString("\tdw {}\n\n", binaryLabel);
                spriteDataPatch.insertString("freedata cleaned\n");
                spriteDataPatch.insertString("{}:\n", binaryLabel);
                spriteDataPatch.insertString("\t!newDataPointer = {}\n", binaryLabel);
                spriteDataPatch.insertString("\tincbin {}\n", binFile.Path());
                spriteDataPatch.insertString("{}_end:\n", binaryLabel);
                spriteDataPatch.insertString("\tprint \"Data pointer  $\",hex(!oldDataPointer),\" : $\",hex(!newDataPointer)\n");
                spriteDataPatch.insertString("\tprint \"Data size     $\",hex(!oldDataSize),\" : $\",hex({}_end-{}-1)\n", binaryLabel, binaryLabel);

                if (debug) {
                    fmt::print("__________________________________\n"); 
                    fmt::print("Fixing sprite data for level {:X}", lv);
                }

                if (!patch(spriteDataPatch, rom, cfg, binFile)) {
                    fmt::print("An error occured when patching sprite data with asar.");
                    return validate(revert);
                }

                if (debug) {
                    fmt::print("Done!\n");
                }

                remapped[lv] = true;
            }
        }

        if (debug) {
            fmt::print("__________________________________\n");
        }

        printf("Sprite data remapped successfully.\n");
        revert = false;
    }
    return validate(revert);
}