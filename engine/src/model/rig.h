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


struct Model;

struct RigGraph {
	void init(const Model& model);

	std::vector<RigNode> nodes;
	RigNodeId root = RigNodeId::invalid();
};

