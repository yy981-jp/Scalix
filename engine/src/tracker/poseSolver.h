#pragma once

#include <array>
#include <vector>

#include <core/avatar.h>
#include <tracker/pose.h>


struct PoseBone {
	HBT humanoidBone;
	BoneConnection landmarks;
	NodeHandle node;
	NodeHandle childNode;
	float restLength;
	float landmarkRestLength = 0.0f;
	vec3f restDirection;
	Quat restRotation;
};

namespace PoseSolverMode {
	constexpr uint8_t upperBody = 1 << 0,
					  lowerBody = 1 << 1;
}

class PoseSolver {
	Avatar& avatar;
	std::vector<PoseBone> bones;
	std::array<PoseLandmark, landmarkCount> previous;
	bool hasPrevious = false;
	float smoothing = 0.35f;
	float minimumVisibility = 0.35f;

	void smoothAndRepair(PoseFrame& frame);
	void constrainBoneLengths(PoseFrame& frame);
	
public:
	explicit PoseSolver(Avatar& avatar);
	
	void setMode(uint8_t mode = PoseSolverMode::upperBody);
	void setSmoothing(float value);
	void setMinimumVisibility(float value);
	void solve(const PoseFrame& frame);

	const std::vector<PoseBone>& getBones() const { return bones; }
};
