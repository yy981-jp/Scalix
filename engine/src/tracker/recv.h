#pragma once
#include <cstdint>
#include <array>

#include <asio.hpp>

#include <tracker/pose.h>

class Recv {
	std::array<char, 4096> buffer;
	asio::io_context io;
	asio::ip::udp::socket socket;

public:
	Recv(uint16_t port = 51801);
	bool tick(PoseFrame& out);
};