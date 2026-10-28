#pragma once

// #include <mocap/poseSolver.h>
#include <core/key.h>
#include <core/camera.h>

class PoseSolver;


struct GlobalContext {
	KCodes keyStat;
	MouseState mStat;
	Camera& cam;
	CameraType& cam_type;
	PoseSolver* poseSolver;
};
