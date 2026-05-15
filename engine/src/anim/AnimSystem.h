#pragma once

#include "../core/str.h"
#include "../core/vec3.h"
#include "../core/quat.h"
#include "loader.h"

#include <unordered_map>


struct Pose {
    vec3f pos;
    Quat rot;
    vec3f scale;
};

struct PlayingAnim {
	StrHs anim;

	float time = 0.0f;
	float speed = 1.0f;

	bool loop = false;
	bool finished = false;
};

class AnimSystem {
    std::unordered_map<StrHs, AnimRtFmt> anims;
    std::vector<PlayingAnim> playing;

public:
    void init(const std::string& path) {
        anims = loadAnim(path);
    }

    // Pose run(StrHs animName) {
    //     const AnimRtFmt& anim = anims[animName];
    //     playing.push_back({.anim = animName});
    // }

    void update() {
        
    }
};
