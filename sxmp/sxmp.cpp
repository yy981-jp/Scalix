#include "sxmp.h"

#include <chrono>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core/utils/logger.hpp>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/pose_landmarker/pose_landmarker.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "tensorflow/lite/logger.h"


namespace {

	void configureLogging() {
		// Abseil / MediaPipe: WARNING以下を抑制
		absl::InitializeLog();
		absl::SetMinLogLevel(absl::LogSeverityAtLeast::kError);

		// TensorFlow Lite: INFOを抑制
		tflite::LoggerOptions::SetMinimumLogSeverity(
			tflite::TFLITE_LOG_WARNING
		);

		// OpenCV: INFOを抑制
		cv::utils::logging::setLogLevel(
			cv::utils::logging::LOG_LEVEL_WARNING
		);
	}
	mediapipe::Image ToMediaPipeImage(const cv::Mat& rgb) {
		auto frame = std::make_shared<mediapipe::ImageFrame>();
		frame->CopyPixelData(mediapipe::ImageFormat::SRGB, rgb.cols, rgb.rows,
							static_cast<int>(rgb.step), rgb.data,
							mediapipe::ImageFrame::kDefaultAlignmentBoundary);
		return mediapipe::Image(std::move(frame));
	}
}


struct SXMPBE_impl {
	std::unique_ptr<mediapipe::tasks::vision::pose_landmarker::PoseLandmarker> landmarker;
	std::chrono::time_point<std::chrono::steady_clock> start_time;
};


SXMocapBackend::SXMocapBackend(): data(std::make_unique<SXMPBE_impl>()) {
	configureLogging();
}


void SXMocapBackend::init(const std::string& modelpath) {
	auto landmarker_options =
		std::make_unique<mediapipe::tasks::vision::pose_landmarker::PoseLandmarkerOptions>();
	landmarker_options->base_options.model_asset_path = modelpath;
	landmarker_options->running_mode = mediapipe::tasks::vision::core::VIDEO;

	auto landmarker_or =
		mediapipe::tasks::vision::pose_landmarker::PoseLandmarker::Create(std::move(landmarker_options));
	if (!landmarker_or.ok()) {
		throw std::runtime_error("PoseLandmarker creation failed: " + landmarker_or.status().ToString());
	}
	data->landmarker = std::move(landmarker_or).value();

	data->start_time = std::chrono::steady_clock::now();
}


void SXMocapBackend::tick(std::span<SXMPLandmark> result, const cv::Mat& frame) {
	cv::Mat rgb;
	switch (frame.channels()) {
		case 1:
			cv::cvtColor(frame, rgb, cv::COLOR_GRAY2RGB);
			break;
		case 3:
			cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
			break;
		case 4:
			cv::cvtColor(frame, rgb, cv::COLOR_BGRA2RGB);
			break;
		default:
			throw std::runtime_error(
				std::string{"Unsupported camera frame channel count: "} +
				std::to_string(frame.channels()));
	}

	const auto elapsed_ms = 
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - data->start_time).count();
	const std::int64_t timestamp_ms =
		std::max<std::int64_t>(elapsed_ms, previous_timestamp_ms + 1);
	previous_timestamp_ms = timestamp_ms;

	auto mp_result_or = data->landmarker->DetectForVideo(ToMediaPipeImage(rgb), timestamp_ms);
	if (!mp_result_or.ok())
		throw std::runtime_error("PoseLandmarker inference failed: " + mp_result_or.status().ToString());

	const auto mp_result = std::move(mp_result_or).value();

	if (!mp_result.pose_world_landmarks.size()) {
		for (auto& el: result) {
			el.visibility = 0;
		}
		return;
	}

	for (size_t i = 0; i < result.size(); i++) {
		const auto lm = mp_result.pose_world_landmarks[0].landmarks[i];
		auto& rlm = result[i];

		rlm.x = lm.x;   rlm.y = lm.y;   rlm.z = lm.z;
		rlm.visibility = lm.visibility? lm.visibility.value() : 0;
	}
}


SXMocapBackend::~SXMocapBackend() {
	const auto close_status = data->landmarker->Close();
	if (!close_status.ok())
		throw std::runtime_error(std::string{"PoseLandmarker close failed: "} + close_status.ToString());
}