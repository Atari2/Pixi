#pragma once
#include "../Rom.h"

class MeiMei {
private:
    std::string name;
    Rom prev;
    ByteArray<uint8_t, 0x400> prevEx{};
    ByteArray<uint8_t, 0x400> nowEx{};
    bool always;
    bool debug;
    bool keepTemp;
    std::string sa1DefPath{};

    bool patch(MemoryFile<char>& patch_name, Rom& rom, PixiConfig& cfg, MemoryFile<char>& binfile);
    int run(Rom& rom, PixiConfig& cfg);
public:
    MeiMei(const MeiMeiConfig& cfg, const std::string& name);
    int validate(bool revert);
    bool overSize(int size);
    int run(PixiConfig& cfg);
    void configureSa1Def(const std::string& pathToSa1Def);
    ~MeiMei() = default;
};