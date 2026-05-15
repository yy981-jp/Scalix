#pragma once

#include "../../../shared/format.h"
#include "../core/str.h"
#include "../core/json.h"


using AnimRtFmt = Format<StrHs>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimRtFmt,
	sampleRate, tracks)
