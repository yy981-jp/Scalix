#pragma once

#include "../../../shared/format.h"
#include "../core/str.h"
#include "../core/json.h"
#include "../core/def.h"
#include "../core/vec2.h"
#include "../core/vec3.h"



using AnimRtFmt_base = Format<NodeId>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base,
	sampleRate, tracks)

struct AnimRtFmt {
	AnimRtFmt_base fmt;
	float end;
};
