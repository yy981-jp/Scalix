#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <core/avatar.h>
#include <model/humanoid.h>
#include <tracker/pose.h>


namespace PoseSolverMode {
	constexpr uint8_t upperBody = 1 << 0,
					  lowerBody = 1 << 1;
}


struct BoneState {
    vec3f restDir;
	Quat bindRot;

	// スキニングで実際に使われる「子ボーンのローカル位置オフセット」の正規化方向。
	// デバッグ描画で {0,0,1} 等の決め打ち軸を使うと、モデルのボーン軸の向き
	// (Y軸方向がボーンの長軸、等)の思い込みに依存してしまい、
	// 実際とズレていても気づけない。これを使えば実際のスキニングと同じ
	// 計算過程(親のグローバル回転 × 子のローカルオフセット)で方向を出せる。
	vec3f childLocalDir;
};

class PoseSolver {
	Avatar& avatar;
	std::array<BoneState, static_cast<size_t>(HBT::Count)> bones;

public:
	explicit PoseSolver(Avatar& avatar);
	
	void solve(const PoseFrame& frame);

	void debug_(const PoseFrame& frame);
};
