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
	static unsigned long long receivedFrame = 0;
	++receivedFrame;

	for (size_t i = 0; i < landmarkCount; ++i) {
		const auto& landmark = frame.landmarks(static_cast<int>(i));
		// mediapipe -> scalix 座標系 変換
		out.landmarks[i] = {
			.pos = {
				landmark.x(),
				-landmark.y(),
				-landmark.z()
			},
			.visibility = landmark.visibility(),
		};
		if (motionTraceEnabled() && (!std::isfinite(landmark.x()) || !std::isfinite(landmark.y()) || !std::isfinite(landmark.z()))) {
			printf("MOTION recv=%llu INVALID_LANDMARK index=%zu raw=(%g,%g,%g)\\n", receivedFrame, i, landmark.x(), landmark.y(), landmark.z());
		}
	}

	// if (motionTraceEnabled()) {
	// 	const auto& s = frame.landmarks(static_cast<int>(LandmarkId::LeftShoulder));
	// 	const auto& e = frame.landmarks(static_cast<int>(LandmarkId::LeftElbow));
	// 	const auto& w = frame.landmarks(static_cast<int>(LandmarkId::LeftWrist));
	// 	printf("MOTION recv=%llu raw S=(%.5f,%.5f,%.5f;v=%.3f) E=(%.5f,%.5f,%.5f;v=%.3f) W=(%.5f,%.5f,%.5f;v=%.3f) scalix S=(%.5f,%.5f,%.5f) E=(%.5f,%.5f,%.5f) W=(%.5f,%.5f,%.5f)\\n", receivedFrame, s.x(),s.y(),s.z(),s.visibility(), e.x(),e.y(),e.z(),e.visibility(), w.x(),w.y(),w.z(),w.visibility(), out.landmarks[(size_t)LandmarkId::LeftShoulder].pos.x,out.landmarks[(size_t)LandmarkId::LeftShoulder].pos.y,out.landmarks[(size_t)LandmarkId::LeftShoulder].pos.z, out.landmarks[(size_t)LandmarkId::LeftElbow].pos.x,out.landmarks[(size_t)LandmarkId::LeftElbow].pos.y,out.landmarks[(size_t)LandmarkId::LeftElbow].pos.z, out.landmarks[(size_t)LandmarkId::LeftWrist].pos.x,out.landmarks[(size_t)LandmarkId::LeftWrist].pos.y,out.landmarks[(size_t)LandmarkId::LeftWrist].pos.z);
	// }

	return true;
}
