#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/collider.h>
#include <physics/util.h>


class PhysicsSystem {
	SpringBoneSystem springBoneSys;
	ColliderSystem colliderSys;


public:
	void init(Avatar& avatar);

	void tick(float dt) {
		springBoneSys.tick(dt);
	}
};
