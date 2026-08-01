#include <tracker/recv.h>

#include <engine/generated/proto/pose.pb.h>



Recv::Recv(uint16_t port):
	socket(
		io,
		asio::ip::udp::endpoint(
			asio::ip::udp::v4(),
			port
		)
	) {}

bool Recv::tick(PoseFrame& out) {
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
		// mediapipe -> scalix 座標系 変換
		out.landmarks[i] = {
			.pos = {
				landmark.x(),
				-landmark.y(),
				landmark.z()
			},
			.visibility = landmark.visibility(),
		};
	}

	return true;
}
