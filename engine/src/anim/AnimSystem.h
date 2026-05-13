#pragma once

#include "../core/sid.h"
#include "../core/vec3.h"
#include "../core/quat.h"
#include "loader.h"

#include <unordered_map>


struct Pose {
    vec3f pos;
    Quat rot;
};

class AnimSystem {
    std::unordered_map<StId, AnimRtFmt> anims;

public:
    void init(const std::string& path) {
        anims = loadAnim(path);
    }

    void run(StId animName) {
        const AnimRtFmt& anim = anims[animName];

    }
};
