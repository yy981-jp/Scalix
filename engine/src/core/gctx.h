#pragma once

// #include <tracker/poseSolver.h>
#include <core/key.h>
#include <core/camera.h>

class PoseSolver;


struct GameContext {
	KCodes& keyStat;
	MouseState& mStat;
	Camera& cam;
	CameraType& cam_type;
	PoseSolver* poseSolver;
};
