#include <physics/physics.h>
#include <util/fmutil.h>


// Physics node type
enum class PNT {
	hairRoot,
	skirtRoot,
};

struct PNCand {
	NodeHandle hdl;

};



void PhysicsSystem::init(Avatar& avatar) {
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,113)) )); // back d l
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,117)) )); // back d r
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,105)) )); // back c l
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,109)) )); // back c r
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,120)) )); // back e l
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,123)) )); // back e r
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,98)) )); // back a
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,101)) )); // back b
	// springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatar.id,199)) )); // 


	std::unordered_map<NodeHandle, PNT> nodes;

	// root 検出
	for (const auto& node: avatar.model.nodes) {
		if (!node.name.hash) continue;
		const auto& name = strsv().get(node.name);
		const auto& tokens = tokenizer(name);


		PNT type;
		bool entry = false;
		for (const auto& token: tokens) {
			if (token == "root") entry = true;
			
			if (token == "hair") type = PNT::hairRoot;
			// if (token == "skirt") type = PNT::skirtRoot;
		}

		if (entry) nodes[avatar.model.nodeHandles[node.id]];
	}


	for (const auto& [hdl,type]: nodes) {
		for (const auto& child: nodeReg.get(hdl).children) {
			const auto& chain = SpringBoneChain( detectBonePath(child) );
			springBoneSystem.add(chain);
		}
	}
}
