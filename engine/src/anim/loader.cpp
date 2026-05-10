#include "loader.h"


std::vector<Format> loadAnim(const std::string& path) {
    std::vector<Format> anims;
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
    for (const json& e: j["body"]) {
        anims.emplace_back(e);
    }

    return anims;
}
