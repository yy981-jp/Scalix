#include "def/node.h"
#include <tracker/pose.h>
#include <tracker/poseSolver.h>

#include <debug/debugDraw.h>

#include <core/nodeRegistry.h>
#include <util/fmutil.h>
#include <def/str.h>


struct LandmarkConnection {
	LandmarkId a;
	LandmarkId b;
};


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

const std::vector<HumanoidConnection> upperBody_list = {
	HumanoidConnection{HBT::leftUpperArm, LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
	// HumanoidConnection{HBT::leftLowerArm, LandmarkId::LeftElbow, LandmarkId::LeftWrist},
	// HumanoidConnection{HBT::rightUpperArm, LandmarkId::RightShoulder, LandmarkId::RightElbow},
	// HumanoidConnection{HBT::rightLowerArm, LandmarkId::RightElbow, LandmarkId::RightWrist},
};

const std::vector<HumanoidConnection> lowerBody_list = {
	HumanoidConnection{HBT::leftUpperLeg, LandmarkId::LeftHip, LandmarkId::LeftKnee},
	HumanoidConnection{HBT::leftLowerLeg, LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
	HumanoidConnection{HBT::rightUpperLeg, LandmarkId::RightHip, LandmarkId::RightKnee},
	HumanoidConnection{HBT::rightLowerLeg, LandmarkId::RightKnee, LandmarkId::RightAnkle},
};

const std::vector<std::vector<HumanoidConnection>> humanoidConnections = {
	upperBody_list,
	lowerBody_list,
};


constexpr LandmarkConnection poseConnections[] = {
    // 顔（目）
    {LandmarkId::Nose, LandmarkId::LeftEyeInner},
    {LandmarkId::LeftEyeInner, LandmarkId::LeftEye},
    {LandmarkId::LeftEye, LandmarkId::LeftEyeOuter},
    {LandmarkId::Nose, LandmarkId::RightEyeInner},
    {LandmarkId::RightEyeInner, LandmarkId::RightEye},
    {LandmarkId::RightEye, LandmarkId::RightEyeOuter},

    // 顔（耳・口）
    {LandmarkId::LeftEar, LandmarkId::MouthLeft},
    {LandmarkId::RightEar, LandmarkId::MouthRight},
    {LandmarkId::MouthLeft, LandmarkId::MouthRight},

    // 上半身（両肩）
    {LandmarkId::LeftShoulder, LandmarkId::RightShoulder},

    // 腕・手（左）
    {LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
    {LandmarkId::LeftElbow, LandmarkId::LeftWrist},
    {LandmarkId::LeftWrist, LandmarkId::LeftPinky},
    {LandmarkId::LeftWrist, LandmarkId::LeftIndex},
    {LandmarkId::LeftWrist, LandmarkId::LeftThumb},
    {LandmarkId::LeftPinky, LandmarkId::LeftIndex},

    // 腕・手（右）
    {LandmarkId::RightShoulder, LandmarkId::RightElbow},
    {LandmarkId::RightElbow, LandmarkId::RightWrist},
    {LandmarkId::RightWrist, LandmarkId::RightPinky},
    {LandmarkId::RightWrist, LandmarkId::RightIndex},
    {LandmarkId::RightWrist, LandmarkId::RightThumb},
    {LandmarkId::RightPinky, LandmarkId::RightIndex},

    // 胴体（肩〜腰）
    {LandmarkId::LeftShoulder, LandmarkId::LeftHip},
    {LandmarkId::RightShoulder, LandmarkId::RightHip},
    {LandmarkId::LeftHip, LandmarkId::RightHip},

    // 脚・足（左）
    {LandmarkId::LeftHip, LandmarkId::LeftKnee},
    {LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
    {LandmarkId::LeftAnkle, LandmarkId::LeftHeel},
    {LandmarkId::LeftHeel, LandmarkId::LeftFootIndex},
    {LandmarkId::LeftAnkle, LandmarkId::LeftFootIndex},

    // 脚・足（右）
    {LandmarkId::RightHip, LandmarkId::RightKnee},
    {LandmarkId::RightKnee, LandmarkId::RightAnkle},
    {LandmarkId::RightAnkle, LandmarkId::RightHeel},
    {LandmarkId::RightHeel, LandmarkId::RightFootIndex},
    {LandmarkId::RightAnkle, LandmarkId::RightFootIndex},
};


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	for (const auto& cnct : upperBody_list) {
		NodeHandle& nodeHdl = avatar.humanoid.bones[(size_t)cnct.bone];
		NodeHandle& parentHdl = avatar.humanoid.bones[ static_cast<size_t>(Humanoid::parentTable[(size_t)cnct.bone]) ];

		bones[(size_t)cnct.bone].restDir =
			( avatar.trackingTransforms[nodeReg.getId(nodeHdl)].pos
			- avatar.trackingTransforms[nodeReg.getId(parentHdl)].pos
			).normalized();
	}
}


void PoseSolver::solve(const PoseFrame& frame) {
    debug_(frame);


	for (const auto& cnct: upperBody_list) {
		// const auto& cnct = upperBody_list[0];

		const vec3f& parent =
			frame.landmarks[(size_t)cnct.parent].pos;
		const vec3f& child =
			frame.landmarks[(size_t)cnct.child].pos;

		const vec3f& currentDir = (child - parent).normalized();
		const vec3f& restDir = bones[(size_t)cnct.bone].restDir;

		Node& node =
			nodeReg.get(avatar.humanoid.bones[(size_t)cnct.bone]);

		// ===== Debug Draw =====

		vec3f origin = parent + vec3f{0, 2, 0};

		// 赤 = Tポーズ方向
		debug.drawLine(
			origin,
			origin + restDir * 0.3f,
			0xff0000ff
		);

		// 緑 = MediaPipe方向
		debug.drawLine(
			origin,
			origin + currentDir * 0.3f,
			0xff00ff00
		);

		node.trs.rot = Quat::fromTo(restDir, currentDir);

		// puts("==============================");

		// vec3f check = node.trs.rot * restDir;

		// printf(
		// 	"rest    = %.3f %.3f %.3f\n"
		// 	"current = %.3f %.3f %.3f\n"
		// 	"quat    = %.3f %.3f %.3f %.3f\n"
		// 	"check   = %.3f %.3f %.3f\n",
		// 	restDir.x,restDir.y,restDir.z,
		// 	currentDir.x,currentDir.y,currentDir.z,
		// 	node.trs.rot.x,
		// 	node.trs.rot.y,
		// 	node.trs.rot.z,
		// 	node.trs.rot.w,
		// 	check.x, check.y, check.z
		// );
	}
}


void PoseSolver::debug_(const PoseFrame& frame) {
	for (auto& lm : frame.landmarks) {
		if (lm.visibility < 0.5) continue;
		debug.drawCross(lm.pos + vec3f{0,2,0}, 0.01f, 0xffff0000u);
	}

	for (const auto& c : poseConnections) {
		debug.drawLine(
			frame.landmarks[static_cast<size_t>(c.a)].pos + vec3f{0,2,0},
			frame.landmarks[static_cast<size_t>(c.b)].pos + vec3f{0,2,0}
		);
	}
}
