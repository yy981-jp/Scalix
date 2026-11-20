#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/collider.h>
#include <physics/util.h>


struct CollideInfo {

};


class PhysicsSystem {
	ColliderSystem colliderSys;


public:
	void init(Avatar& avatar, const LoadData& ld);

	void tick(float dt) {
		colliderSys.tick();
	}
};


extern PhysicsSystem phSys;
