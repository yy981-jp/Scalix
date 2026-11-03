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
	const Model& model = avatar.model;
	for (const auto& spring: model.vrm.springBone.springs) {
		std::vector<NodeHandle> chain;
		chain.reserve(spring.joints.size());
		for (const auto& joint: spring.joints) {
			chain.push_back(model.nodeHandles[joint.node]);
		}

		// SpringBoneChain chain(chain,);
	}


	// for (const auto& [hdl,type]: nodes) {
	// 	for (const auto& child: nodeReg.get(hdl).children) {
	// 		const auto& chain = SpringBoneChain(
	// 			detectBonePath(child),
	// 			0.5f, 0.5f
	// 		);
	// 		springBoneSystem.add(chain);
	// 	}
	// }
}
