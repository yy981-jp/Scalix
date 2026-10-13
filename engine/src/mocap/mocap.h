#pragma once

#include <asio.hpp>

#include <util/path.h>

#include <engine/generated/proto/instr.pb.h>
#include <sxmp/sxmp.h>


class Mocap {
	SXMocapBackend backend;

public:
	Mocap() {
		// backend.init(syspath.data)
	}
};