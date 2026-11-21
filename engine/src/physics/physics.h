#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/collider.h>


struct CollideInfo {

};


struct PhysicsSystem {
	ColliderSystem colliderSys;


	void init(Avatar& avatar, const LoadData& ld);

	void tick(float dt) {
		colliderSys.tick();
	}
};


extern PhysicsSystem phSys;
