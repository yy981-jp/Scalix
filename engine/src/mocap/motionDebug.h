#pragma once

#include <cstdlib>
#include <cstdio>

// Opt-in, concise motion-pipeline tracing for replayed input frames.
inline bool motionTraceEnabled() {
	static const bool enabled = [] {
		const char* value = std::getenv("SCALIX_MOTION_TRACE");
		const bool trace = value != nullptr && value[0] != '\0' && value[0] != '0';
		if (trace) std::setvbuf(stdout, nullptr, _IONBF, 0);
		return trace;
	}();
	return enabled;
}
