#pragma once

#include <cstdint>
#include "camera.h"


struct GameContext {
	uint64_t& keyStat;
	Camera& cam;
};
