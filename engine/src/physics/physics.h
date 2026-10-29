#pragma once
#include <core/avatarSystem.h>
#include <physics/springBone.h>
#include <physics/util.h>


class PhysicsSystem {
	SpringBoneSystem springBoneSystem;


public:
	void init(Avatar& avatar) {
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,113)) )); // back d l
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,117)) )); // back d r
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,105)) )); // back c l
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,109)) )); // back c r
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,120)) )); // back e l
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,123)) )); // back e r
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,98)) )); // back a
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,101)) )); // back b
		springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,199)) )); // 

	}


	void tick(float dt) {
		springBoneSystem.tick(dt);
	}
};
