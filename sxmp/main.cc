
#include <iostream>
#include <memory>
#include <utility>

#include "mediapipe/tasks/cc/vision/pose_landmarker/pose_landmarker.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <pose_landmarker.task>\n";
        return 2;
    }

    using mediapipe::tasks::vision::pose_landmarker::
        PoseLandmarker;
    using mediapipe::tasks::vision::pose_landmarker::
        PoseLandmarkerOptions;

    auto options = std::make_unique<PoseLandmarkerOptions>();
    options->base_options.model_asset_path = argv[1];

    auto result = PoseLandmarker::Create(std::move(options));

    if (!result.ok()) {
        std::cerr << "PoseLandmarker creation failed: "
                  << result.status().ToString() << '\n';
        return 1;
    }

    auto landmarker = std::move(result).value();

    std::cout << "PoseLandmarker initialized successfully.\n";

    const auto close_status = landmarker->Close();
    if (!close_status.ok()) {
        std::cerr << "PoseLandmarker close failed: "
                  << close_status.ToString() << '\n';
        return 1;
    }

    std::cout << "PoseLandmarker closed successfully.\n";
    return 0;
}
