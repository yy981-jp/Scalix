#pragma once

#include <stop_token>

#include <opencv2/opencv.hpp>
#include <asio.hpp>

#include <sxmp/sxmp.h>


class Mocap {
	SXMocapBackend backend;
	cv::VideoCapture cap;
	cv::Mat frame;
	std::vector<SXMPLandmark> landmarks;

	asio::io_context io;
	asio::ip::udp::socket socket;
	asio::ip::udp::endpoint endpoint;
	std::string serialized;


	void update();
	void push();

public:
	Mocap();

	void tick() {
		update();
		push();
	}
};


void mocapWorker(std::stop_token stoken);
