#pragma once
#include <def/id.h>
#include <def/node.h>


struct RigNodeIdTag {};
using RigNodeId = Id<RigNodeIdTag>;

struct RigNode {
	NodeHandle source;
	RigNodeId parent = RigNodeId::invalid();
	std::vector<RigNodeId> children;
};


enum class RigNodeFlag : uint32_t {
	None = 0,
	Skeleton = 1u << 0,
	Physics = 1u << 1,
};


struct Model;

struct RigGraph {
	void init(const Model& model);

	std::vector<RigNode> nodes;
	RigNodeId root = RigNodeId::invalid();
};

