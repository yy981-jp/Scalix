#pragma once

#include <vector>

#include <def/node.h>
#include <def/quat.h>
#include <def/vec3.h>
#include <physics/def.h>
#include <physics/collider.h>
#include <loader/def.h>
#include <model/model.h>


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
	SpringBoneColliderShape shape = SpringBoneColliderShape::Sphere;
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

		SpringBoneParams params;
	};

	Avatar* avatar = nullptr;
	NodeHandle center;
	bool hasCenter = false;
	uint32_t rootDepth = 0;

	std::vector<JointState> joints;
	// std::vector<SpringBoneCollider> colliders;

	void update(float dt);

	friend class SpringBoneSystem;

public:
	SpringBoneChain(
		const std::vector<NodeHandle>& nodes,
		const std::vector<SpringBoneParams>& params,
		NodeHandle center,
		const std::vector<ColliderHandle>& colliders
	);
};


class SpringBoneSystem {
	std::vector<SpringBoneChain> chains;
	std::vector<ColliderHandle> entriedColliders;

public:
	void add(SpringBoneChain&& chain);
	void init(Avatar& avatar, const LoadData& ld);
	// void clear();
	void tick(float dt);

	~SpringBoneSystem();
};
