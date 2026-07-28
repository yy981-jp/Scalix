#include <tracker/poseSolver.h>

#include <algorithm>

#include <core/nodeRegistry.h>
#include <util/quatutil.h>


namespace {

constexpr float directionEpsilon = 0.0001f;

struct HumanoidConnection {
	HBT bone;
	LandmarkId parent;
	LandmarkId child;
};

constexpr std::array humanoidConnections {
	HumanoidConnection{HBT::arm_left_up, LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
	HumanoidConnection{HBT::arm_left_low, LandmarkId::LeftElbow, LandmarkId::LeftWrist},
	HumanoidConnection{HBT::arm_right_up, LandmarkId::RightShoulder, LandmarkId::RightElbow},
	HumanoidConnection{HBT::arm_right_low, LandmarkId::RightElbow, LandmarkId::RightWrist},
	HumanoidConnection{HBT::leg_left_up, LandmarkId::LeftHip, LandmarkId::LeftKnee},
	HumanoidConnection{HBT::leg_left_low, LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
	HumanoidConnection{HBT::leg_right_up, LandmarkId::RightHip, LandmarkId::RightKnee},
	HumanoidConnection{HBT::leg_right_low, LandmarkId::RightKnee, LandmarkId::RightAnkle},
};

bool hasDirection(const vec3f& direction) {
	return bx::length(direction) > directionEpsilon;
}

vec3f position(const Avatar& avatar, NodeHandle node) {
	const auto& mtx = avatar.globalTransforms[nodeReg.getId(node)].mtx;
	return {mtx[12], mtx[13], mtx[14]};
}

vec3f directionInParentSpace(const Avatar& avatar, NodeHandle node, const vec3f& direction) {
	NodeHandle parent = nodeReg.get(node).parent;
	if (!parent.isValid()) return direction;

	const auto& parentMtx = avatar.globalTransforms[nodeReg.getId(parent)].mtx;
	float inverse[16];
	bx::mtxInverse(inverse, parentMtx.data());
	return vec3f{bx::mulXyz0(direction, inverse)};
}

NodeHandle firstChild(NodeHandle node) {
	const Node& source = nodeReg.get(node);
	return source.children.empty() ? NodeHandle::invalid() : source.children.front();
}

}


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	createBones();
}

void PoseSolver::setSmoothing(float value) {
	smoothing = std::clamp(value, 0.0f, 1.0f);
}

void PoseSolver::setMinimumVisibility(float value) {
	minimumVisibility = std::clamp(value, 0.0f, 1.0f);
}

void PoseSolver::createBones() {
	for (const HumanoidConnection& connection : humanoidConnections) {
		if (!avatar.humanoid.has(connection.bone)) continue;

		NodeHandle node = avatar.humanoid.bones[static_cast<size_t>(connection.bone)];
		NodeHandle childNode = firstChild(node);
		if (!childNode.isValid()) continue;

		vec3f direction = position(avatar, childNode) - position(avatar, node);
		direction = directionInParentSpace(avatar, node, direction);
		float length = bx::length(direction);
		if (length <= directionEpsilon) continue;

		bones.push_back({
			.humanoidBone = connection.bone,
			.landmarks = {connection.parent, connection.child},
			.node = node,
			.childNode = childNode,
			.restLength = length,
			.restDirection = direction / length,
			.restRotation = nodeReg.get(node).trs.rot,
		});
	}
}

void PoseSolver::smoothAndRepair(PoseFrame& frame) {
	for (size_t i = 0; i < landmarkCount; ++i) {
		PoseLandmark& landmark = frame.landmarks[i];
		const bool reliable = landmark.visibility >= minimumVisibility;

		if (!reliable && hasPrevious) {
			landmark.pos = previous[i].pos;
		} else if (hasPrevious) {
			landmark.pos = vec3f::lerp(previous[i].pos, landmark.pos, smoothing);
		}
	}

	constrainBoneLengths(frame);
	previous = frame.landmarks;
	hasPrevious = true;
}

void PoseSolver::constrainBoneLengths(PoseFrame& frame) {
	for (PoseBone& bone : bones) {
		PoseLandmark& parent = frame.landmarks[landmarkIndex(bone.landmarks.parent)];
		PoseLandmark& child = frame.landmarks[landmarkIndex(bone.landmarks.child)];
		vec3f observed = child.pos - parent.pos;
		float observedLength = bx::length(observed);
		if (parent.visibility >= minimumVisibility &&
			child.visibility >= minimumVisibility &&
			bone.landmarkRestLength <= directionEpsilon &&
			observedLength > directionEpsilon) {
			bone.landmarkRestLength = observedLength;
		}

		if (child.visibility >= minimumVisibility || bone.landmarkRestLength <= directionEpsilon) continue;

		vec3f direction = observed;
		if (!hasDirection(direction)) direction = bone.restDirection;
		else direction = bx::normalize(direction);

		child.pos = parent.pos + direction * bone.landmarkRestLength;
	}
}

void PoseSolver::solve(const PoseFrame& input) {
	PoseFrame frame = input;
	smoothAndRepair(frame);

	for (const PoseBone& bone : bones) {
		const vec3f& parent = frame.landmarks[landmarkIndex(bone.landmarks.parent)].pos;
		const vec3f& child = frame.landmarks[landmarkIndex(bone.landmarks.child)].pos;
		vec3f currentDirection = child - parent;
		if (!hasDirection(currentDirection)) continue;
		currentDirection = directionInParentSpace(avatar, bone.node, currentDirection);
		if (!hasDirection(currentDirection)) continue;

		float rotation[4];
		quatFromTo(rotation, bone.restDirection, bx::normalize(currentDirection));
		Quat poseRotation{rotation[0], rotation[1], rotation[2], rotation[3]};
		nodeReg.get(bone.node).trs.rot = poseRotation * bone.restRotation;
	}
}
