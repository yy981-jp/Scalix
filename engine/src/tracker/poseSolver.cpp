#include <cstdint>
#include <tracker/poseSolver.h>

#include <debug/debugDraw.h>

#include <core/nodeRegistry.h>
#include <util/fmutil.h>
#include <def/str.h>


struct LandmarkConnection {
    uint8_t a;
    uint8_t b;
};

constexpr LandmarkConnection poseConnections[] = {
    {0, 1}, {1, 2}, {2, 3},
    {0, 4}, {4, 5}, {5, 6},

    {7, 9},
    {8, 10},
    {9, 10},

    {11, 12},

    {11, 13}, {13, 15},
    {15, 17}, {15, 19}, {15, 21}, {17, 19},

    {12, 14}, {14, 16},
    {16, 18}, {16, 20}, {16, 22}, {18, 20},

    {11, 23},
    {12, 24},
    {23, 24},

    {23, 25}, {25, 27},
    {27, 29}, {29, 31}, {27, 31},

    {24, 26}, {26, 28},
    {28, 30}, {30, 32}, {28, 32},
};


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	// setMode();
}


void PoseSolver::solve(const PoseFrame& input) {
	for (auto& lm : input.landmarks) {
		if (lm.visibility < 0.5) continue;
		debug.drawCross(lm.pos + vec3f{0,2,0}, 0.01f, 0xffff0000u);
	}

	for (const auto& c : poseConnections) {
		debug.drawLine(
			input.landmarks[c.a].pos + vec3f{0,2,0},
			input.landmarks[c.b].pos + vec3f{0,2,0}
		);
	}


	
}
