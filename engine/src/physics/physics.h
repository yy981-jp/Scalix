#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/util.h>


class PhysicsSystem {
	SpringBoneSystem springBoneSystem;


public:
	void init(Avatar& avatar);


	void tick(float dt) {
		springBoneSystem.tick(dt);
	}
};
