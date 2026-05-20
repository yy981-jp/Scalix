#pragma once

#include "loader.h"
#include "../core/pose.h"

#include <unordered_map>


struct Avater;
using Value = std::variant<float, vec3f, Quat, bool>;


struct PlayingAnim {
	StrHs anim;

	float time = 0.0f;
	std::vector<int> crtKeyIdxs;
	// float speed = 1.0f;

	bool loop = false;
	// bool finished = false;
};

class AnimSystem {
    std::unordered_map<StrHs, AnimRtFmt> anims;
    std::vector<PlayingAnim> playing;

    std::vector<Value> blendBuffer;
    void blend(float dt);
    void apply(Avater& avater);
    
public:
    void init(const std::string& path, const Avater& avater);

    void run(StrHs animName);
    void stop(StrHs animName);

    void update(Avater& avater, float dt);
};
