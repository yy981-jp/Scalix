#include "loader.h"

#include "../core/avater.h"
#include <unordered_map>


std::unordered_map<StrHs,AnimRtFmt> loadAnim(const std::string& path, const Avater& avater) {
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


    // process pre
    std::unordered_map<StrHs,NodeId> map;
    map.reserve(avater.model.nodes.size());
    for (const Node& node: avater.model.nodes) {
        map[node.name] = node.id;
    }

    // process
    for (const auto& [key,value]: j["body"].items()) {
        const AnimImFmt& imf = value.get<AnimImFmt>();
        AnimRtFmt_base rf_b = imf;
        for (size_t i = 0; i < rf_b.tracks.size(); i++) {
            auto& track_r = rf_b.tracks[i];
            auto& track_i = imf.tracks[i];
            
            auto it = map.find(StrHs(track_i.target));
            if (it == map.end()) continue;
            track_r.target = it->second;
        }

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
