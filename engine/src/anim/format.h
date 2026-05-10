#pragma once

#include "../../../shared/format.h"
#include "../core/sid.h"
#include "../core/json.h"


using RuntimeFormat = Format<StId>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RuntimeFormat::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RuntimeFormat,
	sampleRate, name, tracks)
