#pragma once
#include <cstdint>
#include <array>

#include <engine/generated/proto/pose.pb.h>
#include <asio.hpp>

#include <tracker/pose.h>

class Recv {
	std::array<char, 4096> buffer;
	asio::io_context io;
	asio::ip::udp::socket socket;

public:
	Recv(uint16_t port = 51801):
		socket(
			io,
			asio::ip::udp::endpoint(
				asio::ip::udp::v4(),
				port
			)
		) {}

	bool tick(PoseFrame& out) {
		auto size = socket.receive(
			asio::buffer(buffer)
		);

		sxtr::landmark::pose::PoseFrame frame;

		bool ok = frame.ParseFromArray(
			buffer.data(),
			size
		);

		if (!ok) return false;


		if (frame.landmarks_size() != static_cast<int>(landmarkCount)) return false;

		for (size_t i = 0; i < landmarkCount; ++i) {
			const auto& landmark = frame.landmarks(static_cast<int>(i));
			out.landmarks[i] = {
				.pos = {landmark.x(), landmark.y(), landmark.z()},
				.visibility = landmark.visibility(),
			};
		}

		return true;
	}

};
