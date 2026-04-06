#pragma once

#include <numbers>

class CacheSV {
	static constexpr int TABLE_SIZE = 4096;
	static constexpr int TABLE_MASK = TABLE_SIZE - 1;
	static constexpr float TWO_PI = std::numbers::pi * 2;
	static constexpr float INV_STEP = TABLE_SIZE / TWO_PI;

	float table[TABLE_SIZE][2];

	static int toIndex(float rad);

public:
	static constexpr float OFFSET = std::numbers::pi / 2;

	float getSin(float rad) const;
	float getCos(float rad) const;

	CacheSV();
};

extern CacheSV cachesv;
