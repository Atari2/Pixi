#include "../Rom.h"

class MeiMei {
private:
    static inline std::string name;
    static inline Rom prev;
    static inline ByteArray<uint8_t, 0x400> prevEx{};
    static inline ByteArray<uint8_t, 0x400> nowEx{};
    static inline bool always;
    static inline bool debug;
    static inline bool keepTemp;
    static inline std::string sa1DefPath;

    static bool patch(const std::string& patch_name, Rom& rom, PixiConfig& cfg);
    static int run(Rom& rom, PixiConfig& cfg);
public:
    static void setCfg(const MeiMeiConfig& cfg);
    static void initialize(const std::string& name);
    static int run(PixiConfig& cfg);
    static void configureSa1Def(const std::string& pathToSa1Def);
};