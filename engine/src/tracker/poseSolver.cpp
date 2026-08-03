#include "tracker/pose.h"
#include <cstdint>
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
	// setMode();
}


void PoseSolver::solve(const PoseFrame& frame) {
	debug_(frame);
	
	// for (const auto& cnct: upperBody_list) {
	// 	const vec3f& parent = frame.landmarks[ static_cast<size_t>(cnct.parent) ].pos;
	// 	const vec3f& child = frame.landmarks[ static_cast<size_t>(cnct.child) ].pos;
		
	// 	Node& bone = avatar.model.nodes[static_cast<size_t>(cnct.bone)];
	// 	bone.trs.rot = Quat::fromTo(parent, child);
	// }
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
