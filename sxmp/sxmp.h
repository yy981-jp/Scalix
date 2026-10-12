#pragma once
#include <string>
#include <memory>
#include <cstdint>
#include <chrono>

#include <opencv2/core.hpp>


#if defined(_WIN32)
    #if defined(SXMP_BACKEND_BUILD)
        #define SXMP_API __declspec(dllexport)
    #else
        #define SXMP_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define SXMP_API __attribute__((visibility("default")))
#else
    #define SXMP_API
#endif


struct SXMPBE_impl;

class SXMP_API SXMocapBackend {
	std::unique_ptr<SXMPBE_impl> data;

	const std::string& modelpath;
	int64_t previous_timestamp_ms = -1;

	std::chrono::time_point<std::chrono::steady_clock> start_time;

public:
	SXMocapBackend(const std::string& modelpath);
	void tick(const cv::Mat& frame);
	~SXMocapBackend();
};
