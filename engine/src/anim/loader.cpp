#include "loader.h"


std::unordered_map<StrHs,AnimRtFmt> loadAnim(const std::string& path) {
    std::unordered_map<StrHs,AnimRtFmt> anims;
    json j = readJson(path);

    // check json
    bool invalid = false;
    try {
        if (j.at("version") != 1) invalid = true;
        if (!j.contains("body")) invalid = true;
    } catch (...) {
        invalid = true;
    }

    // process
    for (const auto& [key,value]: j["body"].items()) {
        AnimRtFmt rf = value.get<AnimImFmt>();
        anims[StrHs(key)] = rf;
    }

    return anims;
}
