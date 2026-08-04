#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <core/avatar.h>
#include <tracker/pose.h>


namespace PoseSolverMode {
	constexpr uint8_t upperBody = 1 << 0,
					  lowerBody = 1 << 1;
}


struct BoneState {
    vec3f restDir;
};

class PoseSolver {
	Avatar& avatar;
	std::array<BoneState, static_cast<size_t>(HBT::Count)> bones;

public:
	explicit PoseSolver(Avatar& avatar);
	
	void solve(const PoseFrame& frame);

	void debug_(const PoseFrame& frame);
};
