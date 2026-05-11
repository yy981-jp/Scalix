#include "loader.h"


std::unordered_map<StId,RtAnimFmt> loadAnim(const std::string& path) {
    std::unordered_map<StId,RtAnimFmt> anims;
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
        RtAnimFmt rf = value.get<ImAnimFmt>();
        anims[StId(key)] = rf;
    }

    return anims;
}
