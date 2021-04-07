#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>
#include "MeiMei.h"


constexpr auto SPR_ADDR_LIMIT = 0x800;

#define ERR(msg)                                                                                                       \
    {                                                                                                                  \
        printf("Error: %s", msg);                                                                                      \
        goto end;                                                                                                      \
    }

#define ASSERT_SPR_DATA_ADDR_SIZE(val)                                                                                 \
    if (val >= SPR_ADDR_LIMIT)                                                                                         \
        ERR("Sprite data is too large!");

void MeiMei::configureSa1Def(const std::string& pathToSa1Def) {
    std::string escapedPath = escapeDefines(pathToSa1Def);
    MeiMei::sa1DefPath = escapedPath;
}

bool MeiMei::patch(const std::string& patch_name, Rom& rom, PixiConfig& cfg) {
    rom.patch(patch_name, cfg);
    if (MeiMei::debug) {
        int print_count = 0;
        const char* const* prints = asar_getprints(&print_count);
        for (int i = 0; i < print_count; ++i) {
            fmt::print("\t{}\n", prints[i]);
        }
    }
    return true;
}

void MeiMei::setCfg(const MeiMeiConfig& cfg)
{
    MeiMei::always = cfg.always;
    MeiMei::debug = cfg.debug;
    MeiMei::keepTemp = cfg.keep;
}

void MeiMei::initialize(const std::string& rom_name) {
    MeiMei::name = rom_name;
    prev = std::move(Rom{ MeiMei::name });
    prevEx.fill(0x00);
    nowEx.fill(0x00);
    if (prev.read_byte(0x07730F) == 0x42) {
        int addr = prev.snes_to_pc(prev.read_long(0x07730C), false);
        prevEx.write(prev.data().at(addr), prevEx.size());
    }
}

int MeiMei::run(PixiConfig& cfg) {
    Rom rom(MeiMei::name);
    if (!ErrorState::asar_init_wrap()) {
        ErrorState::pixi_error("Error: Asar library is missing or couldn't be initialized, please redownload the tool or add the dll.\n");
    }

    int returnValue = MeiMei::run(rom, cfg);

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
    Rom now(MeiMei::name);
    if (prev.read_byte(0x07730F) == 0x42) {
        int addr = now.snes_to_pc(now.read_long(0x07730C), false);
        nowEx.write(now.data().at(addr), 0x400);
    }

    bool changeEx = false;
    for (int i = 0; i < 0x400; i++) {
        if (prevEx[i] != nowEx[i]) {
            ErrorState::debug("Extra byte counts changed: 0x{:X} {:X} {:X}\n", i, prevEx[i], nowEx[i]);
            changeEx = true;
            break;
        }
    }

    bool revert = changeEx || MeiMei::always;
    if (changeEx) {
        printf("\nExtra bytes change detected\n");
    }

    if (changeEx || MeiMei::always) {
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
                ERR("Sprite Data has invalid address.");
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
                sprCommonData.write(now.data().at(sprAddrPC + prevOfs), 3);
                if (nowOfs >= SPR_ADDR_LIMIT - 3) {
                    ERR("Sprite data is too large!");
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
                        sprCommonData.write(now.data().at(sprAddrPC + prevOfs), 3);
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
                        ASSERT_SPR_DATA_ADDR_SIZE(nowOfs);
                    }
                    for (; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = 0x00;
                        ASSERT_SPR_DATA_ADDR_SIZE(nowOfs);
                    }
                }
                else if (nowEx[sprNum] < prevEx[sprNum]) {
                    changeData = true;
                    for (int i = 3; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = now.read_byte(sprAddrPC + prevOfs + i);
                        ASSERT_SPR_DATA_ADDR_SIZE(nowOfs);
                    }
                }
                else {
                    for (int i = 3; i < nowEx[sprNum]; i++) {
                        sprAllData[nowOfs++] = now.read_byte(sprAddrPC + prevOfs + i);
                        ASSERT_SPR_DATA_ADDR_SIZE(nowOfs);
                    }
                }
                prevOfs += prevEx[sprNum];
            }

            prevOfs++;
            if (changeData) {
                // create sprite data binary
                std::string binaryFileName = fmt::format("_tmp_bin_{:X}.bin", lv);
                FILE* binFile = fileopen(binaryFileName.c_str(), "wb");
                if (sprAllData.size() > 0) {
                    fwrite(sprAllData.ptr_at(0), 1, sprAllData.size(), binFile);
                }
                fclose(binFile);

                // create patch for sprite data binary
                std::string fileName = fmt::format("_tmp_{:X}.asm", lv);
                FILE* spriteDataPatch = fileopen(binaryFileName.c_str(), "wb");
                std::string binaryLabel = fmt::format("SpriteData{:X}", lv);
                std::string levelBankAddress = fmt::format("{:06X}", now.pc_to_snes(0x077100 + lv, false));
                std::string levelWordAddress = fmt::format("{:06X}", now.pc_to_snes(0x02EC00 + lv * 2, false));

                    // create actual asar patch
                fmt::print(spriteDataPatch, "incsrc \"{}\"\n\n", MeiMei::sa1DefPath);
                fmt::print(spriteDataPatch, "!oldDataPointer = read2(${})|(read1(${})<<16)\n", levelWordAddress, levelBankAddress);
                fmt::print(spriteDataPatch, "!oldDataSize = read2(pctosnes(snestopc(!oldDataPointer)-4))+1\n");
                fmt::print(spriteDataPatch, "autoclean !oldDataPointer\n\n");
                fmt::print(spriteDataPatch, "org ${}\n", levelBankAddress);
                fmt::print(spriteDataPatch, "\tdb {}>>16\n\n", binaryLabel);
                fmt::print(spriteDataPatch, "org ${}\n", levelWordAddress);
                fmt::print(spriteDataPatch, "\tdw {}\n\n", binaryLabel);
                fmt::print(spriteDataPatch, "freedata cleaned\n");
                fmt::print(spriteDataPatch, "{}:\n", binaryLabel);
                fmt::print(spriteDataPatch, "\t!newDataPointer = {}\n", binaryLabel);
                fmt::print(spriteDataPatch, "\tincbin {}\n", binaryFileName);
                fmt::print(spriteDataPatch, "{}_end:\n", binaryLabel);
                fmt::print(spriteDataPatch, "\tprint \"Data pointer  $\",hex(!oldDataPointer),\" : $\",hex(!newDataPointer)\n");
                fmt::print(spriteDataPatch, "\tprint \"Data size     $\",hex(!oldDataSize),\" : $\",hex({}_end-{}-1)\n", binaryLabel, binaryLabel);

                fclose(spriteDataPatch);

                if (MeiMei::debug) {
                    fmt::print("__________________________________\n"); 
                    fmt::print("Fixing sprite data for level {:X}", lv);
                }

                if (!MeiMei::patch(fileName.c_str(), rom, cfg)) {
                    ERR("An error occured when patching sprite data with asar.")
                }

                if (MeiMei::debug) {
                    fmt::print("Done!\n");
                }

                if (!MeiMei::keepTemp) {
                    remove(binaryFileName.c_str());
                    remove(fileName.c_str());
                }

                remapped[lv] = true;
            }
        }

        if (MeiMei::debug) {
            fmt::print("__________________________________\n");
        }

        printf("Sprite data remapped successfully.\n");
        revert = false;
    }
end:
    if (revert) {
        return 1;
    }

    return 0;
}