#pragma once

#include <asio.hpp>

#include <array>
#include <cstdint>
#include <mutex>
#include <thread>

#include <tracker/pose.h>


class Recv {
public:
	Recv(uint16_t port = 51801);
	~Recv();

	bool tick(PoseFrame& out);

private:
	void run();
	void startReceive();

	void handleReceive(
		const asio::error_code& ec,
		std::size_t size
	);

	asio::io_context io;
	asio::ip::udp::socket socket;

	std::jthread thread;

	std::array<char, 4096> buffer;
	asio::ip::udp::endpoint sender;

	std::mutex mutex;
	PoseFrame latestFrame;
	bool hasFrame = false;
};
