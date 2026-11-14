#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <bx/math.h>

#include <physics/springBone.h>
#include <core/avatar.h>
#include <core/nodeRegistry.h>
#include <def/mtx.h>
#include <util/math.h>


namespace {


vec3f transformVector(const vec3f& value, const Mtx& matrix) {
	return bx::mulXyz0(value, matrix.data());
}

vec3f toEvaluationPoint(
	const vec3f& worldPoint,
	bool useCenter,
	const Mtx& inverseCenter
) {
	return useCenter ? worldPoint * inverseCenter : worldPoint;
}

vec3f toEvaluationVector(
	const vec3f& worldVector,
	bool useCenter,
	const Mtx& inverseCenter
) {
	// Gravity is defined in world space. Convert it to center space only
	// because the simulated particle positions are stored in that space.
	return useCenter ? transformVector(worldVector, inverseCenter) : worldVector;
}

vec3f toWorldPoint(
	const vec3f& evaluationPoint,
	bool useCenter,
	const Mtx& centerMatrix
) {
	return useCenter ? evaluationPoint * centerMatrix : evaluationPoint;
}

void constrainLength(
	vec3f& tail,
	const vec3f& head,
	float boneLength,
	const vec3f& fallbackDirection
) {
	vec3f direction = tail - head;
	float currentLength = length(direction);

	if (currentLength <= def::epsilon) {
		direction = fallbackDirection;
		currentLength = length(direction);
	}

	if (currentLength <= def::epsilon || boneLength <= def::epsilon) return;
	tail = head + direction * (boneLength / currentLength);
}

void resolveCollision(
	vec3f& tail,
	const vec3f& head,
	float boneLength,
	float hitRadius,
	const SpringBoneCollider& collider,
	const Avatar& avatar,
	bool useCenter,
	const Mtx& inverseCenter,
	float centerScale,
	const vec3f& fallbackDirection
) {
	if (!nodeReg.isAlive(collider.node) || collider.radius <= 0.0f) return;

	const int colliderIndex = static_cast<int>(nodeReg.getId(collider.node));
	if (colliderIndex < 0 || colliderIndex >= static_cast<int>(avatar.globalMtx.size())) return;

	const Mtx& colliderMatrix = avatar.globalMtx[colliderIndex];
	const vec3f colliderHeadWorld = collider.offset * colliderMatrix;
	const vec3f colliderHead = toEvaluationPoint(
		colliderHeadWorld, useCenter, inverseCenter
	);

	vec3f closestPoint = colliderHead;
	if (collider.shape == ColliderShape::Capsule) {
		const vec3f colliderTailWorld = collider.tail * colliderMatrix;
		const vec3f colliderTail = toEvaluationPoint(
			colliderTailWorld, useCenter, inverseCenter
		);
		const vec3f segment = colliderTail - colliderHead;
		const float segmentLengthSquared = bx::dot(segment, segment);

		if (segmentLengthSquared > def::epsilon) {
			const float t = std::clamp(
				bx::dot(tail - colliderHead, segment) / segmentLengthSquared,
				0.0f,
				1.0f
			);
			closestPoint = colliderHead + segment * t;
		}
	}

	const float colliderScale = maxAbsComponent(colliderMatrix.scale())
		/ std::max(centerScale, def::epsilon);
	const float radius = collider.radius * colliderScale + hitRadius;
	const vec3f delta = tail - closestPoint;
	const float distance = length(delta);

	if (distance >= radius) return;

	vec3f normal = distance > def::epsilon ? delta / distance : fallbackDirection;
	if (length(normal) <= def::epsilon) normal = {0.0f, 1.0f, 0.0f};
	else normal.normalize();

	tail = closestPoint + normal * radius;
	// Collision projection can change the segment length. Reapply the
	// distance constraint exactly as in the reference algorithm.
	constrainLength(tail, head, boneLength, fallbackDirection);
}

} // namespace


SpringBoneChain::SpringBoneChain(
	Avatar& avatar,
	const std::vector<NodeHandle>& nodes,
	const std::vector<SpringBoneParams>& params,
	NodeHandle center,
	const std::vector<SpringBoneCollider>& colliders
):
	avatar(&avatar),
	center(center),
	hasCenter(center.isValid()),
	colliders(colliders) {

	if (nodes.size() < 2) {
		throw std::invalid_argument("SpringBoneChain requires at least two joints");
	}
	if (params.size() != nodes.size() - 1) {
		throw std::invalid_argument("SpringBoneChain parameter count does not match joint pairs");
	}

	for (NodeHandle handle: nodes) {
		if (!nodeReg.isAlive(handle) || nodeReg.getAvatar(handle) != &avatar) {
			throw std::invalid_argument("SpringBoneChain contains an invalid or foreign node");
		}
	}

	if (hasCenter) {
		if (!nodeReg.isAlive(center) || nodeReg.getAvatar(center) != &avatar) {
			throw std::invalid_argument("SpringBone center is invalid or belongs to another avatar");
		}

		bool isAncestor = false;
		std::unordered_set<NodeHandle> visited;
		for (NodeHandle current = nodes.front(); nodeReg.isAlive(current); ) {
			if (!visited.insert(current).second) break;
			if (current == center) {
				isAncestor = true;
				break;
			}
			const NodeHandle parent = nodeReg.get(current).parent;
			if (!parent.isValid()) break;
			current = parent;
		}
		if (!isAncestor) {
			throw std::invalid_argument("SpringBone center must be the first joint or one of its ancestors");
		}
	}

	Mtx inverseCenter = Mtx::fromTRS(Transform{});
	if (hasCenter) {
		const int centerIndex = static_cast<int>(nodeReg.getId(center));
		if (centerIndex < 0 || centerIndex >= static_cast<int>(avatar.globalMtx.size())) {
			throw std::invalid_argument("SpringBone center has no global transform");
		}
		inverseCenter = Mtx::inverse(avatar.globalMtx[centerIndex]);
	}

	for (size_t i = 0; i + 1 < nodes.size(); ++i) {
		bool isAncestor = false;
		std::unordered_set<NodeHandle> visited;
		for (NodeHandle current = nodes[i + 1]; nodeReg.isAlive(current); ) {
			if (!visited.insert(current).second) break;
			if (current == nodes[i]) {
				isAncestor = true;
				break;
			}
			const NodeHandle parent = nodeReg.get(current).parent;
			if (!parent.isValid()) break;
			current = parent;
		}
		if (!isAncestor) {
			throw std::invalid_argument("SpringBone joints must follow the node hierarchy");
		}

		const int headIndex = static_cast<int>(nodeReg.getId(nodes[i]));
		const int tailIndex = static_cast<int>(nodeReg.getId(nodes[i + 1]));
		if (headIndex < 0 || tailIndex < 0
			|| headIndex >= static_cast<int>(avatar.globalMtx.size())
			|| tailIndex >= static_cast<int>(avatar.globalMtx.size())) {
			throw std::invalid_argument("SpringBone joint has no global transform");
		}

		const Node& headNode = nodeReg.get(nodes[i]);
		const Mtx& headMatrix = avatar.globalMtx[headIndex];
		const vec3f tailPosition = avatar.globalMtx[tailIndex].pos();

		// Store the child direction in the head joint's local coordinates.
		const Mtx inverseHead = Mtx::inverse(headMatrix);
		vec3f boneAxis = tailPosition * inverseHead;
		const float axisLength = length(boneAxis);
		if (axisLength <= def::epsilon) {
			throw std::invalid_argument("SpringBone joint pair has zero length");
		}
		boneAxis = boneAxis / axisLength;

		const vec3f headEvaluation = toEvaluationPoint(
			headMatrix.pos(), hasCenter, inverseCenter
		);
		const vec3f tailEvaluation = toEvaluationPoint(
			tailPosition, hasCenter, inverseCenter
		);
		const float boneLength = length(tailEvaluation - headEvaluation);
		if (boneLength <= def::epsilon) {
			throw std::invalid_argument("SpringBone joint pair has zero length in its evaluation space");
		}

		JointState state;
		state.head = nodes[i];
		state.tail = nodes[i + 1];
		state.prevTail = tailEvaluation;
		state.currTail = tailEvaluation;
		state.boneAxis = boneAxis;
		state.boneLength = boneLength;
		state.baseRotation = headNode.mtx.rot();
		state.lastAppliedRotation = headNode.mtx.rot();
		state.params = params[i];
		joints.push_back(state);
	}

	// A smaller depth means a joint closer to the model root. The system
	// sorts chains by this value so parent chains are evaluated first.
	std::unordered_set<NodeHandle> ancestors;
	for (NodeHandle current = nodes.front(); nodeReg.isAlive(current); ) {
		if (!ancestors.insert(current).second) break;
		++rootDepth;
		const NodeHandle parent = nodeReg.get(current).parent;
		if (!parent.isValid()) break;
		current = parent;
	}
}


void SpringBoneChain::prepareFrame() {
	for (JointState& state: joints) {
		if (!nodeReg.isAlive(state.head)) continue;
		Node& node = nodeReg.get(state.head);

		// If an animator changed this joint since the previous physics tick,
		// use that new pose as the spring's base pose. Otherwise undo only
		// the rotation written by the previous spring update.
		if (!Quat::isSameRotation(node.mtx.rot(), state.lastAppliedRotation)) {
			state.baseRotation = node.mtx.rot();
		}

		node.mtx.setRot(state.baseRotation);
		state.lastAppliedRotation = state.baseRotation;
	}
}


void SpringBoneChain::update(float dt) {
	if (!avatar || joints.empty()) return;

	Mtx centerMatrix = Mtx::fromTRS(Transform{});
	Mtx inverseCenter = Mtx::fromTRS(Transform{});
	float centerScale = 1.0f;

	if (hasCenter) {
		const int centerIndex = static_cast<int>(nodeReg.getId(center));
		if (centerIndex < 0 || centerIndex >= static_cast<int>(avatar->globalMtx.size())) return;

		centerMatrix = avatar->globalMtx[centerIndex];
		centerScale = maxAbsComponent(centerMatrix.scale());
		if (centerScale <= def::epsilon) return;
		inverseCenter = Mtx::inverse(centerMatrix);
	}

	for (JointState& state: joints) {
		if (!nodeReg.isAlive(state.head)) continue;

		const int headIndex = static_cast<int>(nodeReg.getId(state.head));
		if (headIndex < 0 || headIndex >= static_cast<int>(avatar->globalMtx.size())) continue;

		const Mtx headMatrix = avatar->globalMtx[headIndex];
		const vec3f headPosition = toEvaluationPoint(
			headMatrix.pos(), hasCenter, inverseCenter
		);

		// The rest direction follows the current animated parent and the
		// joint's un-simulated local rotation.
		vec3f restDirection = transformVector(state.boneAxis, headMatrix);
		restDirection = toEvaluationVector(
			restDirection, hasCenter, inverseCenter
		);
		if (length(restDirection) <= def::epsilon) restDirection = {0.0f, -1.0f, 0.0f};
		else restDirection.normalize();

		const float drag = std::clamp(state.params.dragForce, 0.0f, 1.0f);
		const float damping = std::pow(1.0f - drag, dt * 60.0f);
		const vec3f inertia = (state.currTail - state.prevTail) * damping;

		// Gravity is a world-space force even when a center space is used.
		const vec3f gravityWorld = state.params.gravityDir * state.params.gravityPower;
		const vec3f gravity = toEvaluationVector(
			gravityWorld, hasCenter, inverseCenter
		);

		state.prevTail = state.currTail;

		vec3f nextTail = state.currTail
			+ inertia
			+ restDirection * (std::max(0.0f, state.params.stiffness) * dt)
			+ gravity * dt;

		constrainLength(nextTail, headPosition, state.boneLength, restDirection);

		const float jointScale = maxAbsComponent(headMatrix.scale())
			/ std::max(centerScale, def::epsilon);
		const float hitRadius = std::max(0.0f, state.params.hitRadius) * jointScale;

		for (const SpringBoneCollider& collider: colliders) {
			resolveCollision(
				nextTail,
				headPosition,
				state.boneLength,
				hitRadius,
				collider,
				*avatar,
				hasCenter,
				inverseCenter,
				centerScale,
				restDirection
			);
		}

		state.currTail = nextTail;

		const vec3f nextTailWorld = toWorldPoint(
			nextTail, hasCenter, centerMatrix
		);
		const Mtx inverseHead = Mtx::inverse(headMatrix);
		vec3f targetDirection = nextTailWorld * inverseHead;
		if (length(targetDirection) <= def::epsilon) continue;
		targetDirection.normalize();

		Node& headNode = nodeReg.get(state.head);
		headNode.mtx.setRot( state.baseRotation
			* Quat::fromTo(state.boneAxis, targetDirection) );
		state.lastAppliedRotation = headNode.mtx.rot();

		std::vector<bool> visited(avatar->model.nodes.size(), false);
		avatar->refreshSubtree(state.head);
	}
}


void SpringBoneSystem::add(SpringBoneChain&& chain) {
	if (avatar && avatar != chain.avatar) {
		throw std::invalid_argument("SpringBoneSystem cannot mix avatars");
	}

	avatar = chain.avatar;
	chains.emplace_back(std::move(chain));
	std::stable_sort(
		chains.begin(),
		chains.end(),
		[](const SpringBoneChain& a, const SpringBoneChain& b) {
			return a.rootDepth < b.rootDepth;
		}
	);
}


void SpringBoneSystem::clear() {
	chains.clear();
	avatar = nullptr;
}


void SpringBoneSystem::tick(float dt) {
	if (!avatar || chains.empty() || !std::isfinite(dt) || dt <= 0.0f) return;

	// Avoid an unstable Verlet step after a debugger pause or a long frame.
	dt = std::min(dt, 1.0f / 30.0f);

	for (SpringBoneChain& chain: chains) {
		chain.prepareFrame();
	}
	avatar->refreshSubtree();

	// Chains are ordered from ancestors to descendants. A solved parent
	// refreshes its subtree before a dependent child chain is evaluated.
	for (SpringBoneChain& chain: chains) {
		chain.update(dt);
	}
}
