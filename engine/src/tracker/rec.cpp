#include <iostream>

#include <engine/generated/proto/pose.pb.h>
#include <asio.hpp>

int main() {
	asio::io_context io;

	asio::ip::udp::socket socket(
		io,
		asio::ip::udp::endpoint(
			asio::ip::udp::v4(),
			51801
		)
	);

	std::array<char, 4096> buffer;

	while (true) {
		auto size = socket.receive(
			asio::buffer(buffer)
		);

		sxtr::landmark::pose::PoseFrame frame;

		frame.ParseFromArray(
			buffer.data(),
			size
		);

		std::cout
			<< frame.landmarks_size()
			<< "\n";
	}
}
