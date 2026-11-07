#pragma once

#include <vector>
#include <deque>

#include <model/model.h>
#include <core/gctx.h>
#include <core/avatar.h>


/// @brief avatar 全体管理&描画システム
class AvatarSystem {
	std::unordered_map<AvatarId, Avatar> avatars;
	AvatarId playableAvatar = AvatarId::invalid();

public:
	void loadData(AvatarId id);
	void update(GlobalContext& gctx, float dt);
	void draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones);

	Avatar& player();
	Avatar& get(const AvatarId& id);
};
