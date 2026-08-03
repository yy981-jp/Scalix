#include <cstdint>
#include <tracker/poseSolver.h>

#include <algorithm>
#include <cmath>


#include <debugdraw/debugdraw.h>

#include <core/nodeRegistry.h>
#include <util/quatutil.h>
#include <util/fmutil.h>
#include <def/str.h>


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	// setMode();
}


void PoseSolver::solve(const PoseFrame& input) {
	for (auto& lm : input.landmarks) {
		if (lm.visibility < 0.5) continue;
		debugDraw.drawCross(lm.pos + vec3f{0,1,0}, 0.01f, 0xffff0000u);
	}
}
