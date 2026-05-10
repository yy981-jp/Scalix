#include "loader.h"


std::vector<RuntimeFormat> loadAnim(const std::string& path) {
    std::vector<RuntimeFormat> anims;
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
        RuntimeFormat rf = e.get<ImportFormat>();
        anims.push_back(rf);
    }

    return anims;
}
