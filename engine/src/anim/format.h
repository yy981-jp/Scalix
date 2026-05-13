#pragma once

#include "../../../shared/format.h"
#include "../core/sid.h"
#include "../core/json.h"


using AnimRtFmt = Format<StId>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt,
	sampleRate, tracks)
