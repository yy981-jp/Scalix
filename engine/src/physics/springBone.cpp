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

} // namespace


SpringBoneChain::SpringBoneChain(
	Avatar* avatar,
	const std::vector<NodeHandle>& nodes,
	const std::vector<SpringBoneParams>& params,
	NodeHandle center,
	const std::vector<ColliderHandle>& probes
):
	center(center),
	hasCenter(center.isValid()),
	avatar(avatar) {

	const size_t pairCount = nodes.size() - 1;

	if (nodes.size() < 2)
		throw std::runtime_error("joint.size() < 2");

	if (params.size() < pairCount || probes.size() != pairCount)
		throw std::runtime_error("Invalid SpringBone pair data");

	Mtx inverseCenter;
	if (hasCenter)
		inverseCenter = Mtx::inverse(nodeReg.getGlobalMtx(center));

	for (size_t i = 0; i < pairCount; ++i) {
		const NodeHandle& headNHdl = nodes[i];
		const NodeHandle& tailNHdl = nodes[i + 1];

		const Mtx& headMatrix = nodeReg.getGlobalMtx(headNHdl);
		const vec3f tailPosition = nodeReg.getGlobalMtx(tailNHdl).pos();

		const Mtx inverseHead = Mtx::inverse(headMatrix);
		vec3f boneAxis = tailPosition * inverseHead;

		const float axisLength = length(boneAxis);
		if (axisLength <= def::epsilon)
			throw std::invalid_argument("SpringBone joint pair has zero length");

		boneAxis = boneAxis / axisLength;

		const vec3f headEvaluation = toEvaluationPoint(
			headMatrix.pos(), hasCenter, inverseCenter
		);
		const vec3f tailEvaluation = toEvaluationPoint(
			tailPosition, hasCenter, inverseCenter
		);

		const float boneLength = length(tailEvaluation - headEvaluation);
		if (boneLength <= def::epsilon)
			throw std::invalid_argument(
				"SpringBone joint pair has zero length in evaluation space"
			);

		JointState state;
		state.head = headNHdl;
		state.tail = tailNHdl;
		state.prevTail = tailEvaluation;
		state.currTail = tailEvaluation;
		state.boneAxis = boneAxis;
		state.boneLength = boneLength;
		state.params = params[i];
		state.colHdl = probes[i];
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

		auto& colSys = phSys.colliderSys;
		// colSys.tick();
		for (auto& contact: colSys.getContact(state.colHdl)) {
			nextTail += contact.normal * contact.penetration;
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
		shape.radius = vrmcl.radius;

		col.data = shape;
		return col;
	}
	func(CapsuleCollider) {
		Collider col;
		CLSH::Capsule shape;
		shape.offset = vrmcl.offset;
		shape.radius = vrmcl.radius;
		shape.tail = vrmcl.tail;

		col.data = shape;
		return col;
	}
};
#undef func


} // namespace


void SpringBoneSystem::init(Avatar& avatar, const LoadData& ld) {
	const auto& springBone = ld.vrm.springBone;
	auto& colSys = phSys.colliderSys;

	// ==========================================
	// 1. VRM ColliderのLayerを構築
	// ==========================================

	std::vector<DynamicBitset> layers(springBone.colliders.size());

	for (size_t groupIdx = 0;
		groupIdx < springBone.colliderGroups.size();
		++groupIdx) {

		const auto& group = springBone.colliderGroups[groupIdx];

		for (const auto& colIdx : group.colliders) {
			layers[colIdx.value].set(groupIdx);
		}
	}

	// ==========================================
	// 2. VRM Colliderを登録
	// ==========================================

	for (size_t i = 0; i < springBone.colliders.size(); ++i) {
		const auto& source = springBone.colliders[i];

		Collider col = std::visit(Visitor{}, source.shape);
		col.node = avatar.model.nodeHandles[source.node.value];
		col.layer = std::move(layers[i]);

		ColliderHandle hdl = colSys.add(col);

		// 全Colliderの所有権を管理
		colHandles.push_back(hdl);
	}

	// ==========================================
	// 3. SpringBoneを登録
	// ==========================================

	std::vector<NodeHandle> nodes;
	std::vector<SpringBoneParams> params;
	std::vector<ColliderHandle> probes;

	for (const auto& spring : springBone.springs) {
		const size_t jointSize = spring.joints.size();

		if (jointSize < 2) continue;

		nodes.clear();
		params.clear();
		probes.clear();

		nodes.reserve(jointSize);
		params.reserve(jointSize);
		probes.reserve(jointSize - 1);

		// Springが衝突対象とするLayer
		DynamicBitset mask;

		for (const auto& grIdx : spring.colliderGroups) {
			mask.set(grIdx.value);
		}

		// Joint情報を読み込む
		for (const auto& joint : spring.joints) {
			nodes.push_back(
				avatar.model.nodeHandles[joint.node.value]
			);

			SpringBoneParams param;
			param.stiffness = joint.stiffness;
			param.dragForce = joint.dragForce;
			param.gravityPower = joint.gravityPower;
			param.gravityDir = joint.gravityDir;
			param.hitRadius = joint.hitRadius;

			params.push_back(param);
		}

		// JointPairごとにSpringBone側のSphereを生成
		for (size_t i = 0; i + 1 < jointSize; ++i) {
			Collider probe;

			CLSH::Sphere sphere;
			sphere.radius = spring.joints[i].hitRadius;

			probe.data = sphere;

			// Node追従ではなく、シミュレーション位置を直接更新する
			probe.node = NodeHandle::invalid();
			probe.mask = mask;
			ColliderHandle hdl = colSys.add(probe);

			colHandles.push_back(hdl);
			probes.push_back(hdl);
		}

		NodeHandle center = spring.center.has_value()
			? avatar.model.nodeHandles[spring.center->value]
			: NodeHandle::invalid();

		add(SpringBoneChain(
			&avatar,
			nodes,
			params,
			center,
			probes
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
	for (auto hdl: colHandles) {
		phSys.colliderSys.destroy(hdl);
	}
}
