#include <mocap/mocap.h>

#include <y9inc/time.h>

#include <util/path.h>
#include <def/fs.h>
#include <mocap/pose.h>
#include <proto/pose.pb.h>


void Mocap::update() {
	cap >> frame;
	if (frame.empty()) throw std::runtime_error("Couldn't get frame from camera");
	backend.tick(landmarks, frame);
}

void Mocap::push() {
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

Mocap::Mocap():
	socket(io, asio::ip::udp::v4()),
	endpoint(asio::ip::make_address("127.0.0.1"), 51801)
{
	fs::path modelpath = syspath.resource / "models" / "pose" / "full.task";
	backend.init(modelpath.string());
	cap.open(0);

	landmarks.resize(landmarkCount);
}


void mocapWorker(std::stop_token stoken) {
	Mocap mocap;
	do {
		mocap.tick();
		sleepc(tu::ml, 16);
	} while(!stoken.stop_requested());
}
