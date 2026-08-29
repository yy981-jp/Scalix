#pragma once

#include <asio.hpp>
#include <engine/generated/proto/instr.pb.h>

class Controller {
public:
	Controller(uint16_t port = 51802);
	~Controller();

	bool connect();
	bool send(const sxtr::instr::Instr& instr);

private:
	asio::io_context io;
	asio::ip::tcp::socket socket;

	uint16_t port;
};
