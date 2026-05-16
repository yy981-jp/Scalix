#pragma once

#include "../../../shared/format.h"
#include "../core/str.h"
#include "../core/json.h"


using AnimRtFmt_base = Format<StrHs>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt_base,
	sampleRate, tracks)

struct AnimRtFmt {
	AnimRtFmt_base fmt;
	float end;
};
