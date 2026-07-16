#pragma once

#include <cstdint>
#include <core/key.h>
#include <core/camera.h>


struct GameContext {
	KCodes& keyStat;
	MouseState& mStat;
	Camera& cam;
	CameraType& cam_type;
};
