#pragma once

#include <vector>

#include <def/node.h>
#include <def/quat.h>
#include <def/vec3.h>
#include <physics/def.h>


struct Avatar;

struct SpringBoneParams {
	float stiffness = 1.0f;
	float dragForce = 0.5f;
	float gravityPower = 0.0f;
	vec3f gravityDir{0.0f, -1.0f, 0.0f};
	float hitRadius = 0.0f;
};

struct SpringBoneCollider {
	NodeHandle node;
	ColliderShape shape = ColliderShape::Sphere;
	vec3f offset{};
	vec3f tail{};
	float radius = 0.0f;
};

class SpringBoneChain {
	struct JointState {
		NodeHandle head;
		NodeHandle tail;

		vec3f prevTail{};
		vec3f currTail{};
		vec3f boneAxis{};
		float boneLength = 0.0f;

		Quat baseRotation;
		Quat lastAppliedRotation;
		SpringBoneParams params;
	};

	Avatar* avatar = nullptr;
	NodeHandle center;
	bool hasCenter = false;
	uint32_t rootDepth = 0;

	std::vector<JointState> joints;
	std::vector<SpringBoneCollider> colliders;

	void prepareFrame();
	void update(float dt);

	friend class SpringBoneSystem;

public:
	SpringBoneChain(
		Avatar& avatar,
		const std::vector<NodeHandle>& nodes,
		const std::vector<SpringBoneParams>& params,
		NodeHandle center,
		const std::vector<SpringBoneCollider>& colliders
	);
};


class SpringBoneSystem {
	Avatar* avatar = nullptr;
	std::vector<SpringBoneChain> chains;

public:
	void add(SpringBoneChain&& chain);
	void clear();
	void tick(float dt);
};
