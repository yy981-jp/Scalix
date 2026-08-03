#pragma once

#include <array>
#include <vector>

#include <core/avatar.h>
#include <tracker/pose.h>


namespace PoseSolverMode {
	constexpr uint8_t upperBody = 1 << 0,
					  lowerBody = 1 << 1;
}

class PoseSolver {
	Avatar& avatar;

public:
	explicit PoseSolver(Avatar& avatar);
	
	void solve(const PoseFrame& frame);

	void debug_(const PoseFrame& frame);
};
