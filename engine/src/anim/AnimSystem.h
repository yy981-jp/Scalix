#pragma once

#include "loader.h"
#include "../core/pose.h"

#include <unordered_map>


struct Avater;

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

public:
    void init(const std::string& path);

    void run(StrHs animName);
    void stop(StrHs animName);

    void update(Avater& avater, float dt);
};
