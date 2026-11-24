#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <bx/math.h>


#include <loader/data.h>
#include <physics/physics.h>
#include <physics/springBone.h>
#include <physics/collider.h>
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
	bool useCenter,
	const Mtx& inverseCenter,
	float centerScale,
	const vec3f& fallbackDirection
) {
	const Mtx& colliderMatrix = nodeReg.getGlobalMtx(collider.node);
	const vec3f colliderHeadWorld = collider.offset * colliderMatrix;
	const vec3f colliderHead = toEvaluationPoint(
		colliderHeadWorld, useCenter, inverseCenter
	);

	vec3f closestPoint = colliderHead;
	if (collider.shape == SpringBoneColliderShape::Capsule) {
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
	const std::vector<NodeHandle>& nodes,
	const std::vector<SpringBoneParams>& params,
	NodeHandle center,
	const std::vector<ColliderHandle>& colliders
):
	center(center),
	hasCenter(center.isValid()),
	avatar(avatar) {

	Mtx inverseCenter;
	if (hasCenter)
		inverseCenter = Mtx::inverse(nodeReg.getGlobalMtx(center));

	if (nodes.size() < 2) throw std::runtime_error("joint.size() < 2");

	for (size_t i = 0; i + 1 < nodes.size(); ++i) {
		const NodeHandle& headNHdl = nodes[i];
		const NodeHandle& tailNHdl = nodes[i + 1];

		const Mtx& headMatrix = nodeReg.getGlobalMtx(headNHdl);
		const vec3f tailPosition = nodeReg.getGlobalMtx(tailNHdl).pos();

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


void SpringBoneChain::update(float dt) {
	Mtx centerMatrix, inverseCenter;
	float centerScale = 1.0f;

	if (hasCenter) {
		centerMatrix = nodeReg.getGlobalMtx(center);
		centerScale = maxAbsComponent(centerMatrix.scale());
		if (centerScale <= def::epsilon) return;
		inverseCenter = Mtx::inverse(centerMatrix);
	}

	for (JointState& state: joints) {
		const Mtx headMatrix = nodeReg.getGlobalMtx(state.head);
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

		// for (const SpringBoneCollider& collider: colliders) {
		// 	resolveCollision(
		// 		nextTail,
		// 		headPosition,
		// 		state.boneLength,
		// 		hitRadius,
		// 		collider,
		// 		hasCenter,
		// 		inverseCenter,
		// 		centerScale,
		// 		restDirection
		// 	);
		// }

		state.currTail = nextTail;

		const vec3f nextTailWorld = toWorldPoint(
			nextTail, hasCenter, centerMatrix
		);
		const Mtx inverseHead = Mtx::inverse(headMatrix);
		vec3f targetDirection = nextTailWorld * inverseHead;
		if (length(targetDirection) <= def::epsilon) continue;
		targetDirection.normalize();

		Node& headNode = nodeReg.get(state.head);
		headNode.mtx.rotate( Quat::fromTo(state.boneAxis, targetDirection) );

		avatar->refreshSubtree(state.head);
	}
}


// ========== Sys ==========


void SpringBoneSystem::add(SpringBoneChain&& chain) {
	chains.emplace_back(std::move(chain));
	std::stable_sort(
		chains.begin(),
		chains.end(),
		[](const SpringBoneChain& a, const SpringBoneChain& b) {
			return a.rootDepth < b.rootDepth;
		}
	);
}


namespace {


#define func(Type) Collider operator()(vrm10::Type vrmcl) const
struct Visitor {
	func(SphereCollider) {
		Collider col;
		CLSH::Sphere shape;
		shape.offset = vrmcl.offset;
		shape.radius = shape.radius;

		col.data = shape;
		return col;
	}
	func(CapsuleCollider) {
		Collider col;
		CLSH::Sphere shape;
		shape.offset = vrmcl.offset;
		shape.radius = vrmcl.radius;

		col.data = shape;
		return col;
	}
};
#undef func


} // namespace


void SpringBoneSystem::init(Avatar& avatar, const LoadData& ld) {
	const vrm10::SpringBone& springBone = ld.vrm.springBone;
	
	std::vector<NodeHandle> nodes;
	std::vector<SpringBoneParams> params;
	for (const auto& spring: springBone.springs) {
		int jointSize = spring.joints.size();
		// シミュレーション可能なチェーンか確認
		if (jointSize < 2) continue;

		nodes.clear();	nodes.reserve(jointSize);
		params.clear();	params.reserve(jointSize);

		for (const auto& joint: spring.joints) {
			// NodeIndex -> NodeHandle
			nodes.push_back(avatar.model.nodeHandles[joint.node.value]);
			// VRMデータ -> 物理パラメータ
			SpringBoneParams param;
			param.stiffness = joint.stiffness;
			param.dragForce = joint.dragForce;
			param.gravityPower = joint.gravityPower;
			param.gravityDir = joint.gravityDir;
			param.hitRadius = joint.hitRadius;
			params.push_back(param);
		}

		// ColliderGroup -> Physics Collider
		std::vector<ColliderHandle> colliders;
		for (const auto& grIdx: spring.colliderGroups) {
			const auto& group = springBone.colliderGroups[grIdx.value];

			for (const auto& colIdx: group.colliders) {
				const auto& source = springBone.colliders[colIdx.value];

				// Collider::dataを構築
				Collider col = std::visit(Visitor{}, source.shape);

				col.node = avatar.model.nodeHandles[source.node.value];

				ColliderHandle hdl = phSys.colliderSys.add(col);
				entriedColliders.push_back(hdl);
				colliders.push_back(hdl);
			}
		}


		NodeHandle center = spring.center.has_value()? 
			avatar.model.nodeHandles[spring.center->value] : 
			NodeHandle::invalid();


		// 登録
		add(SpringBoneChain(
			nodes,
			params,
			center,
			colliders
		));
	}
}

void SpringBoneSystem::tick(float dt) {
	if (chains.empty()) return;

	// Chains are ordered from ancestors to descendants. A solved parent
	// refreshes its subtree before a dependent child chain is evaluated.
	for (SpringBoneChain& chain: chains) {
		chain.update(dt);
	}
}

SpringBoneSystem::~SpringBoneSystem() {
	// for (auto hdl: entriedColliders) {
	// 	phSys.colliderSys.destroy(hdl);
	// }
}
