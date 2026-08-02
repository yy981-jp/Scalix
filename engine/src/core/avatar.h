#pragma once

#include <model/model.h>
#include <anim/animSystem.h>
#include <def/euler.h>
#include <def/pose.h>
#include <def/transform.h>


using AvatarId = int;

/// @brief 状態を含む1つのアバター
struct Avatar {
	Model model;
	Humanoid humanoid;

	// Nodes' global transforms. `mtx` is the rendering cache of each TRS.
	std::vector<Transform> globalTransforms;

	// globalTransformsと同じ階層構造だが、entityTransformのyaw/posを除いて計算したもの。
	// モーションキャプチャの入力(カメラ空間、アバターのワールド上の向きとは無関係)を
	// 親ボーン空間に変換する際はこちらを使う。globalTransformsを使うと、アバターが
	// 振り向く(yawが変わる)たびにトラッキングの基準がずれ、本来ローカルなはずの
	// 動きがワールド軸に引きずられてしまう(PoseSolver参照)。
	std::vector<Transform> trackingTransforms;
	

	vec3f pos = {0.0f, 0.0f, 0.0f};
	float yaw = 0.0f;
	float scale[3] = {1.0f, 1.0f, 1.0f};

	Euler head{};
	static constexpr float sensitivity = 0.01f;
	static constexpr float headPitchLimit = 1.2f;
	static constexpr float headYawLimit = 0.5f; // 1.5も良かった

	AnimSystem anim;

	Status status = Status::stay;
	float speed = 7;

	AvatarId id;

	void update(GameContext& keyStat, float dt);
	void draw(Camera& cam);
	
	Avatar(const std::string& glTFPath, AvatarId id);
	~Avatar();

	Avatar(const Avatar&) = delete;
	Avatar& operator=(const Avatar&) = delete;

	Avatar(Avatar&&) noexcept = default;
	Avatar& operator=(Avatar&&) noexcept = default;
};