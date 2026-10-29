#include <physics/physics.h>
#include <util/fmutil.h>


// Physics node type
enum class PNT {
	None,
	hairRoot,
	skirtRoot,
};

struct PNCand {
	NodeHandle hdl;

};



void PhysicsSystem::init(Avatar& avatar) {
	std::unordered_map<NodeHandle, PNT> nodes;

	// root 検出
	for (const auto& node: avatar.model.nodes) {
		if (!node.name.hash) continue;
		const auto& name = strsv().get(node.name);
		const auto& tokens = tokenizer(name);


		PNT type = PNT::None;
		bool entry = false;
		for (const auto& token: tokens) {
			if (token == "root") entry = true;
			
			if (token == "hair") type = PNT::hairRoot;
			// if (token == "skirt") type = PNT::skirtRoot;
		}

		if (entry && type != PNT::None) nodes[avatar.model.nodeHandles[node.id]];
	}


	for (const auto& [hdl,type]: nodes) {
		for (const auto& child: nodeReg.get(hdl).children) {
			const auto& chain = SpringBoneChain( detectBonePath(child) );
			springBoneSystem.add(chain);
		}
	}
}
