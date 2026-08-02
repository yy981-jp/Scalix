#include <cstdint>
#include <tracker/poseSolver.h>

#include <algorithm>

#include <core/nodeRegistry.h>
#include <util/quatutil.h>
#include <util/fmutil.h>
#include <def/str.h>


namespace {

constexpr float directionEpsilon = 0.0001f;

struct HumanoidConnection {
	HBT bone;
	LandmarkId parent;
	LandmarkId child;
};

enum class HMCnsId {
	upperBody,
	lowerBody,

	Count
};

constexpr std::array upperBody_list = {
	HumanoidConnection{HBT::arm_left_up, LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
	HumanoidConnection{HBT::arm_left_low, LandmarkId::LeftElbow, LandmarkId::LeftWrist},
	HumanoidConnection{HBT::arm_right_up, LandmarkId::RightShoulder, LandmarkId::RightElbow},
	HumanoidConnection{HBT::arm_right_low, LandmarkId::RightElbow, LandmarkId::RightWrist},
};

constexpr std::array lowerBody_list = {
	HumanoidConnection{HBT::leg_left_up, LandmarkId::LeftHip, LandmarkId::LeftKnee},
	HumanoidConnection{HBT::leg_left_low, LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
	HumanoidConnection{HBT::leg_right_up, LandmarkId::RightHip, LandmarkId::RightKnee},
	HumanoidConnection{HBT::leg_right_low, LandmarkId::RightKnee, LandmarkId::RightAnkle},
};

constexpr std::array humanoidConnections = {
	upperBody_list,
	lowerBody_list,
};


bool hasDirection(const vec3f& direction) {
	return bx::length(direction) > directionEpsilon;
}

vec3f position(const Avatar& avatar, NodeHandle node) {
	// トラッキングの基準はavatar.yaw/posに依存しないtrackingTransformsを使う。
	// globalTransformsを使うと、アバターが振り向くたびにこの後の
	// directionInParentSpace()の基準もずれてしまう。
	const auto& mtx = avatar.trackingTransforms[nodeReg.getId(node)].mtx;
	return {mtx[12], mtx[13], mtx[14]};
}

vec3f directionInParentSpace(const Avatar& avatar, NodeHandle node, const vec3f& direction) {
	NodeHandle parent = nodeReg.get(node).parent;
	if (!parent.isValid()) return direction;

	// モーションキャプチャ入力(カメラ空間の相対ベクトル)はアバターの
	// ワールド上の向き(yaw)や位置(pos)とは無関係なため、その成分を含まない
	// trackingTransformsの逆行列で親ボーン空間へ変換する。ここで
	// globalTransforms(yaw込み)を使うと、キャリブレーション後にアバターが
	// 振り向いた分だけ基準がずれ、ローカルなはずの動きがワールド軸に
	// 引きずられてしまう(=global化してしまう)。
	const auto& parentMtx = avatar.trackingTransforms[nodeReg.getId(parent)].mtx;
	float inverse[16];
	bx::mtxInverse(inverse, parentMtx.data());
	return vec3f{bx::mulXyz0(direction, inverse)};
}

bool containsSupport(std::string_view name) {
	for (const auto& w : tokenizer(name)) {
		if (w == "support") return true;
	}
	return false;
}

NodeHandle firstChild(NodeHandle node) {
	const Node& source = nodeReg.get(node);
	if (source.children.empty()) return NodeHandle::invalid();

	// "support"を含む名前の子（衣装/腕などの補正ボーン）は関節チェーンの本流ではないため、
	// 他に候補があればスキップして次の実関節ボーンを優先する。
	// 例: Upper_arm.L の children = [Costume_support1.L, Costume_support3.L, Lower_arm.L, Upper_arm_support.L]
	//     -> Lower_arm.L を選びたい
	for (const NodeHandle& child : source.children) {
		std::string_view name = strsv().get(nodeReg.get(child).name);
		if (!containsSupport(name)) return child;
	}

	return source.children.front();
}

}


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	setMode();
}

void PoseSolver::setSmoothing(float value) {
	smoothing = std::clamp(value, 0.0f, 1.0f);
}

void PoseSolver::setMinimumVisibility(float value) {
	minimumVisibility = std::clamp(value, 0.0f, 1.0f);
}

void PoseSolver::setMode(uint8_t mode) {
	bones.clear();

	for (int targetMode = 0; targetMode < static_cast<int>(HMCnsId::Count); targetMode++) {
		if ( !(mode & (1 << targetMode)) ) continue; // 指定されていないフラグは無視

		// flagで指定されたものだけ処理
		for (const HumanoidConnection& connection: humanoidConnections[targetMode]) {
			if (!avatar.humanoid.has(connection.bone)) continue;

			NodeHandle node = avatar.humanoid.bones[static_cast<size_t>(connection.bone)];
			NodeHandle childNode = firstChild(node);
			if (!childNode.isValid()) continue;

			vec3f direction = position(avatar, childNode) - position(avatar, node);
			direction = directionInParentSpace(avatar, node, direction);
			float length = bx::length(direction);
			if (length <= directionEpsilon) continue;

			bones.push_back( {
				.humanoidBone = connection.bone,
				.landmarks = {connection.parent, connection.child},
				.node = node,
				.childNode = childNode,
				.restLength = length,
				.restDirection = direction / length,
				.restRotation = nodeReg.get(node).trs.rot,
				.calibratedDirection = direction / length, // キャリブレーションされるまではrestDirectionにフォールバック
			} );
		}
	}

	needsCalibration = true;
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

void PoseSolver::calibrate(const PoseFrame& frame) {
	bool allOk = true;

	for (PoseBone& bone : bones) {
		const PoseLandmark& parent = frame.landmarks[landmarkIndex(bone.landmarks.parent)];
		const PoseLandmark& child = frame.landmarks[landmarkIndex(bone.landmarks.child)];

		if (parent.visibility < minimumVisibility || child.visibility < minimumVisibility) {
			allOk = false;
			continue;
		}

		vec3f direction = child.pos - parent.pos;
		if (!hasDirection(direction)) { allOk = false; continue; }

		direction = directionInParentSpace(avatar, bone.node, direction);
		if (!hasDirection(direction)) { allOk = false; continue; }

		bone.calibratedDirection = bx::normalize(direction);
	}

	// 全ボーンぶんキャリブレーションできた時だけ完了扱いにする。
	// 一部失敗した場合は次のフレームで再試行する（その間はrestDirection/前回値のまま）。
	if (allOk) needsCalibration = false;
}

void PoseSolver::solve(const PoseFrame& input) {
	PoseFrame frame = input;
	smoothAndRepair(frame);

	if (needsCalibration) calibrate(frame);

	for (const PoseBone& bone : bones) {
		const vec3f& parent = frame.landmarks[landmarkIndex(bone.landmarks.parent)].pos;
		const vec3f& child = frame.landmarks[landmarkIndex(bone.landmarks.child)].pos;
		vec3f currentDirection = child - parent;
		if (!hasDirection(currentDirection)) continue;
		currentDirection = directionInParentSpace(avatar, bone.node, currentDirection);
		if (!hasDirection(currentDirection)) continue;

		float rotation[4];
		// モデルの基準ポーズ(restDirection)ではなく、トラッキング開始時に実際に
		// 観測された姿勢(calibratedDirection)からの相対回転を使う。
		// これによりモデルがTポーズ/Aポーズいずれの基準ポーズであっても、
		// トラッキング対象の人が普段どんな姿勢で立っていても正しく追従する。
		quatFromTo(rotation, bone.calibratedDirection, bx::normalize(currentDirection));
		Quat poseRotation{rotation[0], rotation[1], rotation[2], rotation[3]};
		nodeReg.get(bone.node).trs.rot = poseRotation * bone.restRotation;
	}
}
