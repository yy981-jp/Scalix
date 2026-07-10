#pragma once
#include <chrono>


class ElapsedTime {
public:
    ElapsedTime() {}
	void init();
    float get();
	operator bool();
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	bool enable = false;
};
