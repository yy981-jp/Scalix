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

	// トラッキング開始時（キャリブレーション時）に実際に観測された方向。
	// モデルの基準ポーズ(Tポーズ等)と実際の人の姿勢が一致しない問題に対応するため、
	// quatFromToの基準にはrestDirectionではなくこちらを使う。
	// キャリブレーション未実施の間はrestDirectionと同じ値にしておく。
	vec3f calibratedDirection;
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
	bool needsCalibration = true;
	float smoothing = 0.35f;
	float minimumVisibility = 0.35f;

	void smoothAndRepair(PoseFrame& frame);
	void constrainBoneLengths(PoseFrame& frame);
	void calibrate(const PoseFrame& frame);

public:
	explicit PoseSolver(Avatar& avatar);
	
	void setMode(uint8_t mode = PoseSolverMode::upperBody);
	void setSmoothing(float value);
	void setMinimumVisibility(float value);
	void solve(const PoseFrame& frame);

	// 現在のトラッキング姿勢を新たな基準（ゼロ点）として登録し直す。
	// ユーザーが実際に取っているポーズと、モデルの基準ポーズが食い違う問題に対応する。
	void requestCalibration() { needsCalibration = true; }

	const std::vector<PoseBone>& getBones() const { return bones; }
};
