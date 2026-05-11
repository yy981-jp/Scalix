#pragma once

#include "../../../shared/format.h"
#include "../core/sid.h"
#include "../core/json.h"


using RtAnimFmt = Format<StId>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RtAnimFmt::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RtAnimFmt,
	sampleRate, tracks)
