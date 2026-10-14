#pragma once

#include <opencv2/opencv.hpp>
#include <asio.hpp>

#include <y9inc/time.h>

#include <engine/generated/proto/instr.pb.h>
#include <sxmp/sxmp.h>

#include <util/path.h>
#include <def/fs.h>
#include <mocap/pose.h>
#include <proto/pose.pb.h>


class Mocap {
	SXMocapBackend backend;
	cv::VideoCapture cap;
	cv::Mat frame;
	std::vector<SXMPLandmark> landmarks;

	asio::io_context io;
	asio::ip::udp::socket socket;
	asio::ip::udp::endpoint endpoint;
	std::string serialized;


	void update() {
		cap >> frame;
		if (frame.empty()) throw std::runtime_error("Couldn't get frame from camera");
		backend.tick(landmarks, frame);
	}

	void push() {
		
		sxtr::landmark::pose::PoseFrame poseFrame;
		for (auto& lm: landmarks) {
			auto* pfLm = poseFrame.add_landmarks();
			pfLm->set_x(lm.x);
			pfLm->set_y(lm.y);
			pfLm->set_z(lm.z);
			pfLm->set_visibility(lm.visibility);
		}
		poseFrame.set_timestamp(UINT64_MAX); // TODO: timestampどうするん? 要る?

		serialized.clear();
		if (!poseFrame.SerializeToString(&serialized)) throw std::runtime_error("Mocap::push(): serialize");

		socket.send_to(asio::buffer(serialized), endpoint);
	}

public:
	Mocap():
		socket(io, asio::ip::udp::v4()),
		endpoint(asio::ip::make_address("127.0.0.1"), 51801)
	{
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


inline void mocapWorker(std::stop_token stoken) {
	Mocap mocap;
	while (!stoken.stop_requested()) {
		mocap.tick();
		sleepc(tu::ml, 16);
	}
}
