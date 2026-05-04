#pragma once

#include <cstdint>
#include "key.h"
#include "camera.h"


struct GameContext {
	KCodes& keyStat;
	MouseState& mStat;
	Camera& cam;
	CameraType& cam_type;
};
