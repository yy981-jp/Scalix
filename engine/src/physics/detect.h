#pragma once
#include <model/model.h>
#include <util/fmutil.h>
#include <unordered_map>


// std::array<NodeHandle,static_cast<size_t>(HBT::Count)> bones;


// Physics node type
enum class PNT {
	hairRoot,
	skirtRoot,
};

struct PNCand {
	NodeHandle hdl;

};



class PhysicsNode {
public:
	std::unordered_map<NodeHandle, PNT> nodes;


	void detect(const Model& model) {
		for (const auto& node: model.nodes) {
			const auto& tokens = strsv().get(node.name);
			
			NodeHandle hdl;
			PNT type;

			bool entry = false;
			
			for (const auto& token: tokenizer(tokens)) {
				if (token == "root") entry = true;
				
				if (token == "hair") type = PNT::hairRoot;
				if (token == "skirt") type = PNT::skirtRoot;
			}

			if (entry) nodes[hdl] = type;
		}
	}

	void resolveParam() {
		for (const auto& [hdl, type]: nodes) {
			
		}
	}
};
