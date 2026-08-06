#pragma once

#include <array>
#include <cstddef>

#include <def/vec3.h>


enum class LandmarkId : size_t {
	Nose,
	LeftEyeInner,
	LeftEye,
	LeftEyeOuter,
	RightEyeInner,
	RightEye,
	RightEyeOuter,
	LeftEar,
	RightEar,
	MouthLeft,
	MouthRight,
	LeftShoulder,
	RightShoulder,
	LeftElbow,
	RightElbow,
	LeftWrist,
	RightWrist,
	LeftPinky,
	RightPinky,
	LeftIndex,
	RightIndex,
	LeftThumb,
	RightThumb,
	LeftHip,
	RightHip,
	LeftKnee,
	RightKnee,
	LeftAnkle,
	RightAnkle,
	LeftHeel,
	RightHeel,
	LeftFootIndex,
	RightFootIndex,
	Count,
};

constexpr size_t landmarkCount = static_cast<size_t>(LandmarkId::Count);

struct PoseLandmark {
	vec3f pos;
	float visibility = 0.0f;
};

struct PoseFrame {
	std::array<PoseLandmark, landmarkCount> landmarks;
};

struct BoneConnection {
	LandmarkId parent;
	LandmarkId child;
};
