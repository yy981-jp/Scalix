#include <util/time.h>


void ElapsedTime::init(){
	start = std::chrono::high_resolution_clock::now();
	enable = true;
}

float ElapsedTime::get() {
	std::chrono::time_point<std::chrono::high_resolution_clock> e = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(e - start);
	start = std::move(e);
	return duration.count();
}

ElapsedTime::operator bool() {
	return enable;
}
