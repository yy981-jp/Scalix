#include <tracker/recv.h>

#include <engine/generated/proto/pose.pb.h>
#include <tracker/motionDebug.h>

#include <cmath>
#include <cstdio>

Recv::Recv(uint16_t port):
	socket(
		io,
		asio::ip::udp::endpoint(
			asio::ip::udp::v4(),
			port
		)
	),
	thread([this] {
		run();
	}) {
	startReceive();
}

Recv::~Recv() {
	io.stop();
}

void Recv::run() {
	io.run();
}

void Recv::startReceive() {
	socket.async_receive_from(
		asio::buffer(buffer),
		sender,
		[this](const asio::error_code& ec, std::size_t size) {
			handleReceive(ec, size);
		}
	);
}

void Recv::handleReceive(
	const asio::error_code& ec,
	std::size_t size
) {
	if (!ec) {
		sxtr::landmark::pose::PoseFrame frame;

		bool ok = frame.ParseFromArray(
			buffer.data(),
			size
		);

		if (ok &&
			frame.landmarks_size() == static_cast<int>(landmarkCount)) {

			PoseFrame latest;

			for (size_t i = 0; i < landmarkCount; ++i) {
				const auto& landmark =
					frame.landmarks(static_cast<int>(i));

				// mediapipe -> scalix 座標系 変換
				latest.landmarks[i] = {
					.pos = {
						landmark.x(),
						-landmark.y(),
						-landmark.z()
					},
					.visibility = landmark.visibility(),
				};
			}

			{
				std::lock_guard lock(mutex);

				latestFrame = std::move(latest);
				hasFrame = true;
			}
		}
	}

	startReceive();
}

bool Recv::tick(PoseFrame& out) {
	std::lock_guard lock(mutex);

	if (!hasFrame) {
		return false;
	}

	out = latestFrame;

	return true;
}
