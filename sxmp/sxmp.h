#pragma once
#include <string>
#include <memory>
#include <cstdint>
#include <span>

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


struct SXMPLandmark {
	float x, y, z, visibility;
};

struct SXMPBE_impl;

class SXMP_API SXMocapBackend {
	std::unique_ptr<SXMPBE_impl> data;

	int64_t previous_timestamp_ms = -1;

public:
	SXMocapBackend();
	void init(const std::string& modelpath);
	void tick(std::span<SXMPLandmark> result, const cv::Mat& frame);

	~SXMocapBackend();
};
