#include <tracker/controller.h>

#include <array>
#include <string>

Controller::Controller(uint16_t port)
	: socket(io), port(port) {}

bool Controller::connect() {
	if (socket.is_open())
		return true;

	try {
		asio::ip::tcp::resolver resolver(io);
		auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));

		asio::connect(socket, endpoints);
		return true;
	} catch (const asio::system_error&) {
		return false;
	}
}

bool Controller::send(const sxtr::instr::Instr& instr) {
	if (!socket.is_open())
		return false;

	std::string data;

	if (!instr.SerializeToString(&data))
		return false;

	const uint32_t size = static_cast<uint32_t>(data.size());

	std::array<uint8_t, 4> header{
		static_cast<uint8_t>((size >> 24) & 0xff),
		static_cast<uint8_t>((size >> 16) & 0xff),
		static_cast<uint8_t>((size >> 8) & 0xff),
		static_cast<uint8_t>(size & 0xff)
	};

	try {
		asio::write(socket, asio::buffer(header));
		asio::write(socket, asio::buffer(data));

		return true;
	} catch (const asio::system_error&) {
		return false;
	}
}

Controller::~Controller() {
	if (socket.is_open()) {
		std::error_code ec;
		auto _ =socket.shutdown(
			asio::ip::tcp::socket::shutdown_both,
			ec
		);
		_ = socket.close(ec);
	}
}
