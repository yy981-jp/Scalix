#include "sxmp.h"


#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/pose_landmarker/pose_landmarker.h"


namespace {
	mediapipe::Image ToMediaPipeImage(const cv::Mat& rgb) {
		auto frame = std::make_shared<mediapipe::ImageFrame>();
		frame->CopyPixelData(mediapipe::ImageFormat::SRGB, rgb.cols, rgb.rows,
							static_cast<int>(rgb.step), rgb.data,
							mediapipe::ImageFrame::kDefaultAlignmentBoundary);
		return mediapipe::Image(std::move(frame));
	}
}


struct SXMPBE_impl {
	cv::VideoCapture capture;
	std::unique_ptr<mediapipe::tasks::vision::pose_landmarker::PoseLandmarker> landmarker;
};


SXMocapBackend::SXMocapBackend(const std::string& modelpath): modelpath(modelpath), data(std::make_unique<SXMPBE_impl>()) {
	data->capture.open(0);
	
	if (!data->capture.isOpened()) {
		throw std::runtime_error("Failed to open camera device 0.");
		
	}

	auto landmarker_options =
		std::make_unique<mediapipe::tasks::vision::pose_landmarker::PoseLandmarkerOptions>();
	landmarker_options->base_options.model_asset_path = modelpath;
	landmarker_options->running_mode = mediapipe::tasks::vision::core::VIDEO;

	auto landmarker_or =
		mediapipe::tasks::vision::pose_landmarker::PoseLandmarker::Create(std::move(landmarker_options));
	if (!landmarker_or.ok()) {
		data->capture.release();
		throw std::runtime_error("PoseLandmarker creation failed: " + landmarker_or.status().ToString());
	}
	data->landmarker = std::move(landmarker_or).value();

	start_time = std::chrono::steady_clock::now();
}


void SXMocapBackend::tick(const cv::Mat& frame) {
	if (!data->capture.read(frame) || frame.empty())
		throw std::runtime_error("Camera frame read failed.");

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
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
	const std::int64_t timestamp_ms =
		std::max<std::int64_t>(elapsed_ms, previous_timestamp_ms + 1);
	previous_timestamp_ms = timestamp_ms;

	auto result_or = data->landmarker->DetectForVideo(ToMediaPipeImage(rgb), timestamp_ms);
	if (!result_or.ok())
		throw std::runtime_error("PoseLandmarker inference failed: " + result_or.status().ToString());

	const auto result = std::move(result_or).value();
	
	result.pose_world_landmarks; // TODO: この結果データをどう扱うか
}


SXMocapBackend::~SXMocapBackend() {
	data->capture.release();

	const auto close_status = data->landmarker->Close();
	if (!close_status.ok())
		throw std::runtime_error(std::string{"PoseLandmarker close failed: "} + close_status.ToString());
}