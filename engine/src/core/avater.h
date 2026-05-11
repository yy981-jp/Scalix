#pragma once

#include "../model/model.h"
#include "../anim/animSystem.h"


using AvaterID = int;

/// @brief 状態を含む1つのアバター
struct Avater {
	Model model;
	Humanoid humanoid;

	// nodes mtx
	std::vector<std::array<float, 16>> globalMtxs;

	vec3f pos   = {0.0f, 0.0f, 0.0f};
	float yaw;
	float scale[3] = {1.0f, 1.0f, 1.0f};

	Euler head;
    const float sensitivity = 0.01f;
    const float headPitchLimit = 1.2f;
    const float headYawLimit = 0.5f; // 1.5も良かった

	AnimSystem anim;

	Status status = Status::stay;
	float speed = 0.2;

	AvaterID id;

	void update(GameContext& keyStat);
	void draw(Camera& cam);
	
	Avater(const std::string& glTFPath);
};
