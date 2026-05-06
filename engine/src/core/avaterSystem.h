#pragma once

#include <vector>

#include "../model/model.h"
#include "gctx.h"


/// @brief avater 全体管理&描画システム
class AvaterSystem {
	std::vector<Avater> avaters;
	AvaterID playableAvater = 0;

public:
	void loadData(const std::vector<std::string> path);
	void update(GameContext& gctx);
	void draw(bgfx::ProgramHandle program);
};
