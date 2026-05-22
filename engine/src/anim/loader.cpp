#include "loader.h"

#include "../core/avater.h"
#include <unordered_map>
#include <unordered_set>
#include <fstream>


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
    std::ofstream ofs("test.txt");
    for (const Node& node: avater.model.nodes) {
        map[node.name] = node.id;
        ofs << node.name.hash << "\t\t\t" << strsv().get(node.name) << "\t=\t" << node.id << "\n";
    }

    for (size_t i = 0; i < 10; i++) ofs << "\n";

    ofs << "----------\n";

    for (const auto& node: avater.model.nodes) {
        ofs << strsv().get(node.name) << "\n";
    }

    for (size_t i = 0; i < 10; i++) ofs << "\n";
    
    // --- DEBUG: Track missing node analysis ---
    ofs << "===== TRACK TARGET ANALYSIS =====\n";
    std::unordered_set<std::string> missingTargets;
    std::unordered_set<std::string> foundTargets;
    
    for (const auto& [key,value]: j["body"].items()) {
        const AnimImFmt& imf = value.get<AnimImFmt>();
        for (const auto& track_i: imf.tracks) {
            auto it = map.find(StrHs(track_i.target));
            if (it == map.end()) {
                missingTargets.insert(track_i.target);
            } else {
                foundTargets.insert(track_i.target);
            }
        }
    }
    
    ofs << "Found targets: " << foundTargets.size() << "\n";
    for (const auto& t: foundTargets) {
        ofs << "  OK: " << t << "\n";
    }
    
    ofs << "\nMissing targets: " << missingTargets.size() << "\n";
    for (const auto& t: missingTargets) {
        ofs << "  MISSING: " << t << "\n";
    }
    ofs << "===== END TRACK ANALYSIS =====\n\n";
    

    // process
    for (const auto& [key,value]: j["body"].items()) {
        const AnimImFmt& imf = value.get<AnimImFmt>();
        AnimRtFmt_base rf_b = imf;
        for (size_t i = 0; i < rf_b.tracks.size(); i++) {
            auto& track_r = rf_b.tracks[i];
            auto& track_i = imf.tracks[i];
            
            auto it = map.find(StrHs(track_i.target));
            if (it != map.end()) {
                ofs << "D: found: i-s:" << track_i.target << " i:" << StrHs(track_i.target).hash << " r:" << track_r.target << "\n";
                track_r.target = it->second;
            } else {
                ofs << "D: notfound: i-s:" << track_i.target << " i:" << StrHs(track_i.target).hash << " r:" << track_r.target << "\n";
                track_r.target = -404;
                continue;
            }
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
