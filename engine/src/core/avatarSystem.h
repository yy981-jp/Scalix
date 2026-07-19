#pragma once

#include <vector>
#include <deque>

#include <model/model.h>
#include <core/gctx.h>
#include <core/avatar.h>


/// @brief avatar 全体管理&描画システム
class AvatarSystem {
	std::deque<Avatar> avatars;
	AvatarId playableAvatar = 0;

public:
	void loadData(const std::vector<std::string> path);
	void update(GameContext& gctx, float dt);
	void draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones);
};
