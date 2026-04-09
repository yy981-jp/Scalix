#pragma once

#include <vector>

#include "../model/model.h"


class AvaterSystem {
	std::vector<Avater> avaters;

public:
	void loadData(const std::vector<std::string> path);
	void update(const uint64_t& keyStat);
	void draw(bgfx::ProgramHandle program);
};
