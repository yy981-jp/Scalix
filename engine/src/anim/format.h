#pragma once

#include <shared/format.h>
#include <def/str.h>
#include <def/json.h>
#include <def/def.h>
#include <def/vec2.h>
#include <def/vec3.h>



using AnimRtFmt_base = Format<NodeId>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base,
	sampleRate, tracks)

struct AnimRtFmt {
	AnimRtFmt_base fmt;
	float end;
};
