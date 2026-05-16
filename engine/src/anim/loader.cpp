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

    if (invalid) throw std::runtime_error(".sxa is invalid");

    // process
    for (const auto& [key,value]: j["body"].items()) {
        AnimRtFmt_base rf_b = value.get<AnimImFmt>();
        AnimRtFmt rf;
        rf.fmt = std::move(rf_b);
        anims[StrHs(key)] = rf;
    }

    // calc end
    for (auto& [key,anim]: anims) {
        float longest = 0.f;
        for (const auto& track: anim.fmt.tracks) {
            longest = std::max(longest, track.keys.back().time);
        }
        anim.end = longest;
    }

    return anims;
}
