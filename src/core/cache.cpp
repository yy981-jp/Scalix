#include "cache.h"

#include <cmath>

CacheSV::CacheSV() {
	for (int i = 0; i < TABLE_SIZE; ++i) {
		float rad = TWO_PI * i / TABLE_SIZE;
		table[i][0] = std::cos(rad);
		table[i][1] = std::sin(rad);
	}
}

float CacheSV::getSin(float rad) const {
	int idx = toIndex(rad);
	return table[idx][1];
}

float CacheSV::getCos(float rad) const {
	int idx = toIndex(rad);
	return table[idx][0];
}

int CacheSV::toIndex(float rad) {
	int idx = static_cast<int>(rad * INV_STEP);
	// no offset any more; wrap using the mask
	return idx & TABLE_MASK;
}


CacheSV cachesv;