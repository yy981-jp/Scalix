#include <physics/physics.h>

#include <algorithm>
#include <cstdio>
#include <unordered_set>


namespace {

bool isAncestorOrSelf(NodeHandle ancestor, NodeHandle node) {
	std::unordered_set<NodeHandle> visited;
	for (NodeHandle current = node; nodeReg.isAlive(current); ) {
		if (!visited.insert(current).second) return false;
		if (current == ancestor) return true;

		const NodeHandle parent = nodeReg.get(current).parent;
		if (!parent.isValid()) break;
		current = parent;
	}
	return false;
}

vec3f readVector3(const std::vector<float>& values, const vec3f& fallback) {
	if (values.size() < 3) return fallback;
	return {values[0], values[1], values[2]};
}

void warnSpring(size_t index, const char* reason) {
	std::fprintf(stderr, "[SpringBone] skipping spring %zu: %s\n", index, reason);
}

} // namespace


void PhysicsSystem::init(Avatar& avatar) {
	springBoneSys.clear();

	const Model& model = avatar.model;
	const auto& springBone = model.vrm.springBone;
	if (springBone.springs.empty()) return;

	if (springBone.specVersion != "1.0") {
		std::fprintf(
			stderr,
			"[SpringBone] unsupported specVersion: %s\n",
			springBone.specVersion.c_str()
		);
		return;
	}

	// The synthetic root added by GltfLoader is not a source glTF node and
	// must not be addressable from VRMC_springBone indices.
	size_t sourceNodeCount = model.nodeHandles.size();
	if (nodeReg.isAlive(model.root)) {
		sourceNodeCount = static_cast<size_t>(nodeReg.getId(model.root));
	}

	std::unordered_set<NodeHandle> usedJointNodes;
	std::vector<NodeHandle> usedCenters;

	for (size_t springIndex = 0; springIndex < springBone.springs.size(); ++springIndex) {
		const auto& spring = springBone.springs[springIndex];
		if (spring.joints.size() < 2) continue;

		std::vector<NodeHandle> nodes;
		nodes.reserve(spring.joints.size());
		std::unordered_set<NodeHandle> localJointNodes;
		bool valid = true;

		for (const auto& joint: spring.joints) {
			if (joint.node >= sourceNodeCount) {
				valid = false;
				break;
			}

			const NodeHandle handle = model.nodeHandles[joint.node];
			if (!nodeReg.isAlive(handle)
				|| nodeReg.getAvatar(handle) != &avatar
				|| !localJointNodes.insert(handle).second
				|| usedJointNodes.contains(handle)) {
				valid = false;
				break;
			}
			nodes.push_back(handle);
		}

		if (!valid) {
			warnSpring(springIndex, "invalid or duplicated joint node");
			continue;
		}

		for (size_t i = 0; i + 1 < nodes.size(); ++i) {
			if (detectBonePath(nodes[i], nodes[i + 1]).empty()) {
				valid = false;
				break;
			}
		}
		if (!valid) {
			warnSpring(springIndex, "joints do not follow the node hierarchy");
			continue;
		}

		NodeHandle center = NodeHandle::invalid();
		if (springIndex < model.vrm.springCenters.size()
			&& model.vrm.springCenters[springIndex].has_value()) {
			const uint32_t centerNode = *model.vrm.springCenters[springIndex];
			if (centerNode >= sourceNodeCount) {
				warnSpring(springIndex, "center node index is out of range");
				continue;
			}

			center = model.nodeHandles[centerNode];
			if (!nodeReg.isAlive(center) || !isAncestorOrSelf(center, nodes.front())) {
				warnSpring(springIndex, "center must be the first joint or one of its ancestors");
				continue;
			}

			// A center cannot be a joint or a descendant of a joint in
			// another spring chain.
			bool centerConflict = false;
			for (NodeHandle used: usedJointNodes) {
				if (isAncestorOrSelf(used, center)) {
					centerConflict = true;
					break;
				}
			}
			if (centerConflict) {
				warnSpring(springIndex, "center conflicts with another spring chain");
				continue;
			}
		}

		// Check earlier centers against this chain even if this chain has
		// no center of its own.
		bool previousCenterConflict = false;
		for (NodeHandle previousCenter: usedCenters) {
			for (NodeHandle joint: nodes) {
				if (isAncestorOrSelf(joint, previousCenter)) {
					previousCenterConflict = true;
					break;
				}
			}
			if (previousCenterConflict) break;
		}
		if (previousCenterConflict) {
			warnSpring(springIndex, "a previous center is a joint or descendant of this spring chain");
			continue;
		}

		std::vector<SpringBoneParams> params;
		params.reserve(spring.joints.size() - 1);
		for (size_t i = 0; i + 1 < spring.joints.size(); ++i) {
			const auto& joint = spring.joints[i];

			SpringBoneParams param;
			param.stiffness = std::max(0.0f, joint.stiffness);
			param.dragForce = std::clamp(joint.dragForce, 0.0f, 1.0f);
			param.gravityPower = joint.gravityPower;
			param.gravityDir = readVector3(joint.gravityDir, {0.0f, -1.0f, 0.0f});
			param.hitRadius = std::max(0.0f, joint.hitRadius);
			params.push_back(param);
		}

		std::vector<SpringBoneCollider> colliders;
		std::unordered_set<uint32_t> usedColliderIndices;

		for (uint32_t groupIndex: spring.colliderGroups) {
			if (groupIndex >= springBone.colliderGroups.size()) continue;

			for (uint32_t colliderIndex: springBone.colliderGroups[groupIndex].colliders) {
				if (colliderIndex >= springBone.colliders.size()
					|| !usedColliderIndices.insert(colliderIndex).second) {
					continue;
				}

				const auto& source = springBone.colliders[colliderIndex];
				if (source.node >= sourceNodeCount) continue;

				const NodeHandle colliderNode = model.nodeHandles[source.node];
				if (!nodeReg.isAlive(colliderNode)) continue;

				SpringBoneCollider collider;
				collider.node = colliderNode;

				if (source.shape.capsule.radius > 0.0f) {
					collider.shape = ColliderShape::Capsule;
					collider.offset = readVector3(source.shape.capsule.offset, {});
					collider.tail = readVector3(source.shape.capsule.tail, {});
					collider.radius = source.shape.capsule.radius;
				} else if (source.shape.sphere.radius > 0.0f) {
					collider.shape = ColliderShape::Sphere;
					collider.offset = readVector3(source.shape.sphere.offset, {});
					collider.radius = source.shape.sphere.radius;
				} else {
					continue;
				}

				colliders.push_back(collider);
			}
		}

		try {
			springBoneSys.add(SpringBoneChain(
				avatar,
				nodes,
				params,
				center,
				colliders
			));
			usedJointNodes.insert(localJointNodes.begin(), localJointNodes.end());
			if (center.isValid()) usedCenters.push_back(center);
		} catch (const std::exception& error) {
			std::fprintf(
				stderr,
				"[SpringBone] skipping spring %zu: %s\n",
				springIndex,
				error.what()
			);
		}
	}
}
