#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/collider.h>
#include <physics/util.h>


struct CollideInfo {

};


class PhysicsSystem {
	SpringBoneSystem springBoneSys;
	ColliderSystem colliderSys;


public:
	void init(Avatar& avatar, const LoadData& ld);

	void tick(float dt) {
		springBoneSys.tick(dt);
	}
};


extern PhysicsSystem phSys;
