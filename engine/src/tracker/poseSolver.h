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

	// このボーン自身の「バインドポーズでのグローバル回転」(トラッキング空間)。
	// PoseSolver構築時に一度だけ avatar.trackingTransforms から取得する。
	// ローカル回転(node.trs.rotのオリジナル値)ではなく、
	// 親の回転も含めて合成済みのグローバル回転であることが重要。
	// (親ボーン自身もPoseSolverで動的に回転する場合、bind時の親回転と
	// 現在の親回転は別物になるため、ローカル回転だけを保持して都度
	// 現在の親回転とサンドイッチする方式では計算が破綻する)
	Quat globalBindRot;

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
