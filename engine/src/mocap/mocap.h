#pragma once

#include <opencv2/opencv.hpp>
#include <asio.hpp>

#include <engine/generated/proto/instr.pb.h>
#include <sxmp/sxmp.h>

#include <util/path.h>
#include <def/fs.h>
#include <mocap/pose.h>


class Mocap {
	SXMocapBackend backend;
	cv::VideoCapture cap;
	cv::Mat frame;
	std::vector<SXMPLandmark> landmarks;

	void update() {
		cap >> frame;
		if (frame.empty()) throw std::runtime_error("Couldn't get frame from camera");
		backend.tick(landmarks, frame);
	}

	void push() {
		
	}

public:
	Mocap() {
		fs::path modelpath = syspath.resource / "models" / "pose_landmarker_full.task";
		backend.init(modelpath.string());
		cap.open(0);

		landmarks.reserve(landmarkCount);
	}

	void tick() {
		update();
		tick();
	}
};
