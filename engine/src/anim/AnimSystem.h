#pragma once

#include "../core/sid.h"
#include "loader.h"

#include <unordered_map>


class AnimSystem {
    std::unordered_map<StId, RtAnimFmt> anims;

public:
    void init(const std::string& path) {
        anims = loadAnim(path);
    }

    void run(StId animName) {
        
    }
};
