#include "cache.h"

#include <cmath>

LUTSV::LUTSV() {
	for (int i = 0; i < TABLE_SIZE; ++i) {
		float rad = TWO_PI * i / TABLE_SIZE;
		table[i][0] = std::cos(rad);
		table[i][1] = std::sin(rad);
	}
}

float LUTSV::getSin(float rad) const {
	int idx = toIndex(rad);
	return table[idx][1];
}

float LUTSV::getCos(float rad) const {
	int idx = toIndex(rad);
	return table[idx][0];
}

int LUTSV::toIndex(float rad) {
	int idx = static_cast<int>(rad * INV_STEP);
	// no offset any more; wrap using the mask
	return idx & TABLE_MASK;
}


LUTSV lutsv;