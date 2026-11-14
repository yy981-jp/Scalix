#pragma once

#include <model/model.h>
#include <model/humanoid.h>
#include <model/avatarDef.h>
#include <anim/animSystem.h>
#include <util/jump.h>
#include <def/euler.h>
#include <def/pose.h>
#include <def/transform.h>


/// @brief 状態を含む1つのアバター
struct Avatar {
	Model model;

	// Nodes' global transforms. `mtx` is the rendering cache of each TRS.
	std::vector<Mtx> globalMtx;

	// globalMtxと同じ階層構造だが、entityTransformのyaw/posを除いて計算したもの。
	// モーションキャプチャの入力(カメラ空間、アバターのワールド上の向きとは無関係)を
	// 親ボーン空間に変換する際はこちらを使う。globalMtxを使うと、アバターが
	// 振り向く(yawが変わる)たびにトラッキングの基準がずれ、本来ローカルなはずの
	// 動きがワールド軸に引きずられてしまう(PoseSolver参照)。
	std::vector<Mtx> avatarSpaceMtx;
	

	vec3f pos = {0.0f, 0.0f, 0.0f};
	float yaw = 0.0f;
	vec3f scale = {1.0f, 1.0f, 1.0f};
	Mtx avatarMtx;

	Jump jump;
	const float floor_y = 0;

	Euler head{};
	static constexpr float sensitivity = 0.01f;
	static constexpr float headPitchLimit = 1.2f;
	static constexpr float headYawLimit = 0.5f; // 1.5も良かった

	AnimSystem anim;

	Status status = Status::stay;
	float speed;

	AvatarId id;

	void update(GlobalContext& keyStat, float dt);
	void draw(Camera& cam);
	
	Avatar(const AvatarId& id);
	~Avatar();

	Avatar(const Avatar&) = delete;
	Avatar& operator=(const Avatar&) = delete;

	Avatar(Avatar&&) noexcept = default;


	void refreshSubtree(NodeHandle handle = NodeHandle::invalid()) {
		if (!handle.isValid()) handle = model.root;
		refreshSubtreeLoop++;
		refreshSubtreeImpl(handle,globalMtx,avatarMtx);
	}
	void refreshSubtreeFor(std::vector<Mtx>& tMtx, const Mtx& tMtxRt) {
		refreshSubtreeLoop++;
		refreshSubtreeImpl(model.root,tMtx,tMtxRt);
	}


private:
	std::vector<uint32_t> visited;
	uint32_t refreshSubtreeLoop = 0;

	void refreshSubtreeImpl(const NodeHandle& handle, std::vector<Mtx>& tMtx, const Mtx& tMtxRt);
};
