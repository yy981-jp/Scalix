#pragma once

#include <vector>

#include "../model/model.h"
#include "gctx.h"
#include "avatar.h"


/// @brief avatar 全体管理&描画システム
class AvatarSystem {
	std::vector<Avatar> avatars;
	AvatarID playableAvatar = 0;

public:
	void loadData(const std::vector<std::string> path);
	void update(GameContext& gctx, float dt);
	void draw(bgfx::ProgramHandle program);
	void draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones);
};
