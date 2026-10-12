#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/pose_landmarker/pose_landmarker.h"

namespace {

constexpr float kVisibilityThreshold = 0.5f;

// Connections used by the original Python sxtr-camera implementation.
constexpr int kPoseConnections[][2] = {
	{0, 1},	 {1, 2},	 {2, 3},	 {0, 4},	 {4, 5},	 {5, 6},
	{7, 9},	 {8, 10},	{9, 10},	{11, 12},
	{11, 13}, {13, 15}, {15, 17}, {15, 19}, {15, 21}, {17, 19},
	{12, 14}, {14, 16}, {16, 18}, {16, 20}, {16, 22}, {18, 20},
	{11, 23}, {12, 24}, {23, 24},
	{23, 25}, {25, 27}, {27, 29}, {29, 31}, {27, 31},
	{24, 26}, {26, 28}, {28, 30}, {30, 32}, {28, 32},
};

volatile std::sig_atomic_t g_stop_requested = 0;

void HandleSigInt(int) {
	g_stop_requested = 1;
}

struct Options {
	std::string model_path = "models/pose_landmarker_full.task";
	bool background = false;
};

void PrintUsage(const char* executable) {
	std::cout
		<< "Usage: " << executable
		<< " [<model.task>] [--model <name-or-path>] [--background|-b]\n\n"
		<< "Examples:\n"
		<< "	" << executable
		<< " C:\\cpp\\scalix\\tracker\\models\\pose_landmarker_full.task\n"
		<< "	" << executable << " --model full\n"
		<< "	" << executable << " --model C:\\models\\pose_landmarker_full.task\n\n"
		<< "Without --background, ESC closes the preview window. In background "
				"mode, press Ctrl+C to stop.\n";
}

bool LooksLikeTaskPath(const std::string& value) {
	if (value.find('/') != std::string::npos ||
			value.find('\\') != std::string::npos) {
		return true;
	}
	constexpr char kTaskExtension[] = ".task";
	constexpr std::size_t kExtensionLength = sizeof(kTaskExtension) - 1;
	return value.size() >= kExtensionLength &&
		value.compare(value.size() - kExtensionLength, kExtensionLength,
		kTaskExtension) == 0;
}

std::string ResolveModelArgument(const std::string& value) {
	if (LooksLikeTaskPath(value)) {
		return value;
	}
	return "models/pose_landmarker_" + value + ".task";
}

bool ParseArguments(int argc, char* argv[], Options* options) {
	bool positional_model_seen = false;

	for (int i = 1; i < argc; ++i) {
		const std::string arg(argv[i]);

		if (arg == "--help" || arg == "-h") {
			PrintUsage(argv[0]);
			return false;
		}
		if (arg == "--background" || arg == "-b") {
			options->background = true;
			continue;
		}
		if (arg == "--model") {
			if (i + 1 >= argc) {
				std::cerr << "Missing value after --model.\n";
				PrintUsage(argv[0]);
				return false;
			}
			options->model_path = ResolveModelArgument(argv[++i]);
			continue;
		}
		if (!arg.empty() && arg[0] == '-') {
			std::cerr << "Unknown option: " << arg << '\n';
			PrintUsage(argv[0]);
			return false;
		}
		if (positional_model_seen) {
			std::cerr << "Only one positional model path is accepted.\n";
			PrintUsage(argv[0]);
			return false;
		}

		// A positional argument is always treated as an explicit filesystem path.
		options->model_path = arg;
		positional_model_seen = true;
	}

	return true;
}

using NormalizedLandmark =
	mediapipe::tasks::components::containers::NormalizedLandmark;

bool IsVisible(const NormalizedLandmark& landmark) {
	return landmark.visibility.has_value() &&
		landmark.visibility.value() >= kVisibilityThreshold;
}

cv::Point ToImagePoint(const NormalizedLandmark& landmark, int width, int height) {
	return cv::Point(static_cast<int>(landmark.x * width),
			static_cast<int>(landmark.y * height));
}

void DrawPoseLandmarks(
	cv::Mat& image,
	const mediapipe::tasks::vision::pose_landmarker::PoseLandmarkerResult& result) {
	for (const auto& pose : result.pose_landmarks) {
		const auto& landmarks = pose.landmarks;

		for (const auto& connection : kPoseConnections) {
			const auto a = static_cast<std::size_t>(connection[0]);
			const auto b = static_cast<std::size_t>(connection[1]);
			if (a >= landmarks.size() || b >= landmarks.size()) {
				continue;
			}
			if (!IsVisible(landmarks[a]) || !IsVisible(landmarks[b])) {
				continue;
			}

			cv::line(image, ToImagePoint(landmarks[a], image.cols, image.rows),
							 ToImagePoint(landmarks[b], image.cols, image.rows),
							 cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
		}

		for (std::size_t i = 0; i < landmarks.size(); ++i) {
			const auto& landmark = landmarks[i];
			if (!IsVisible(landmark)) {
				continue;
			}

			const cv::Point point = ToImagePoint(landmark, image.cols, image.rows);
			cv::circle(image, point, 4, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
			cv::putText(image, std::to_string(i), cv::Point(point.x + 5, point.y - 5),
									cv::FONT_HERSHEY_SIMPLEX, 0.35,
									cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
		}
	}
}

mediapipe::Image ToMediaPipeImage(const cv::Mat& rgb) {
	auto frame = std::make_shared<mediapipe::ImageFrame>();
	frame->CopyPixelData(mediapipe::ImageFormat::SRGB, rgb.cols, rgb.rows,
						static_cast<int>(rgb.step), rgb.data,
						mediapipe::ImageFrame::kDefaultAlignmentBoundary);
	return mediapipe::Image(std::move(frame));
}

}	// namespace

int main(int argc, char* argv[]) {
	Options options;
	if (!ParseArguments(argc, argv, &options)) {
		// --help is a successful request; malformed arguments are an error.
		for (int i = 1; i < argc; ++i) {
			const std::string arg(argv[i]);
			if (arg == "--help" || arg == "-h") {
				return 0;
			}
		}
		return 2;
	}

	std::signal(SIGINT, HandleSigInt);

	cv::VideoCapture capture(0);
	if (!capture.isOpened()) {
		std::cerr << "Failed to open camera device 0.\n";
		return 1;
	}

	auto landmarker_options =
		std::make_unique<mediapipe::tasks::vision::pose_landmarker::PoseLandmarkerOptions>();
	landmarker_options->base_options.model_asset_path = options.model_path;
	landmarker_options->running_mode = mediapipe::tasks::vision::core::VIDEO;

	auto landmarker_or =
		mediapipe::tasks::vision::pose_landmarker::PoseLandmarker::Create(std::move(landmarker_options));
	if (!landmarker_or.ok()) {
		std::cerr << "PoseLandmarker creation failed: "
				<< landmarker_or.status().ToString() << '\n';
		capture.release();
		return 1;
	}
	auto landmarker = std::move(landmarker_or).value();

	std::cout << "PoseLandmarker initialized successfully.\n"
				<< "Model: " << options.model_path << '\n'
				<< "Camera: 0\n"
				<< (options.background
					? "Preview disabled; press Ctrl+C to stop.\n"
					: "Preview enabled; press ESC to stop.\n");

	const auto start_time = std::chrono::steady_clock::now();
	std::int64_t previous_timestamp_ms = -1;
	int exit_code = 0;

	try {
		while (!g_stop_requested) {
			cv::Mat frame;
			if (!capture.read(frame) || frame.empty()) {
				if (!g_stop_requested) {
					std::cerr << "Camera frame read failed.\n";
					exit_code = 1;
				}
				break;
			}

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
					std::cerr << "Unsupported camera frame channel count: "
										<< frame.channels() << '\n';
					exit_code = 1;
					g_stop_requested = 1;
					continue;
			}

			const auto elapsed_ms = 
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
			const std::int64_t timestamp_ms =
				std::max<std::int64_t>(elapsed_ms, previous_timestamp_ms + 1);
			previous_timestamp_ms = timestamp_ms;

			auto result_or = landmarker->DetectForVideo(ToMediaPipeImage(rgb), timestamp_ms);
			if (!result_or.ok()) {
				std::cerr << "PoseLandmarker inference failed: "
						<< result_or.status().ToString() << '\n';
				exit_code = 1;
				break;
			}

			if (!options.background) {
				const auto result = std::move(result_or).value();
				DrawPoseLandmarks(frame, result);
				cv::imshow("camera", frame);
				const int key = cv::waitKey(1);
				if ((key & 0xFF) == 27) break;
			}
		}
	} catch (const cv::Exception& error) {
		std::cerr << "OpenCV error: " << error.what() << '\n';
		exit_code = 1;
	}

	capture.release();
	if (!options.background) {
		cv::destroyAllWindows();
	}

	const auto close_status = landmarker->Close();
	if (!close_status.ok()) {
		std::cerr << "PoseLandmarker close failed: "
					<< close_status.ToString() << '\n';
		exit_code = 1;
	} else {
		std::cout << "PoseLandmarker closed successfully.\n";
	}

	return exit_code;
}
