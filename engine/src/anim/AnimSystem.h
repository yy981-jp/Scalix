#pragma once

#include <anim/loader.h>
#include <def/pose.h>

#include <unordered_map>


struct Avatar;
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
	std::vector<bool> blendBufferSR;
	void blend(float dt);
	void apply(Avatar& avatar);
	
public:
	void init(const std::string& path, const Avatar& avatar);

	void run(StrHs animName);
	void stop(StrHs animName);

	void tick(Avatar& avatar, float dt);
};
