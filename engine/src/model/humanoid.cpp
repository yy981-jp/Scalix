#include <model/model.h>
#include <model/humanoid.h>
#include <util/fmutil.h>
#include <loader/data.h>
#include <loader/vrm.h>
#include <core/nodeRegistry.h>


void Humanoid::init(const Model& model, const LoadData& ld) {
	const auto& hm = ld.vrm.vrm.humanoid;

	#define X(name) {HBT::name, hm[vrm10::HumanBone::name]},

	std::vector<std::pair<HBT,std::optional<vrm10::NodeIndex>>> dict {
		X(hips)

		X(spine)
		X(chest)
		X(upperChest)
		X(neck)
		X(head)

		X(leftShoulder)
		X(leftUpperArm)
		X(leftLowerArm)
		X(leftHand)

		X(rightShoulder)
		X(rightUpperArm)
		X(rightLowerArm)
		X(rightHand)

		X(leftUpperLeg)
		X(leftLowerLeg)
		X(leftFoot)
		X(leftToes)

		X(rightUpperLeg)
		X(rightLowerLeg)
		X(rightFoot)
		X(rightToes)
	};

	#undef X


	for (auto [hbt,vrm_hbt]: dict) {
		auto& bone = bones[static_cast<size_t>(hbt)];
		if (!vrm_hbt.has_value()) continue;
		bone = model.nodeHandles[vrm_hbt->value];

		// flag
		if (bone.isValid()) {
			boneFlag |= HBTFlag(hbt);
		}
	}

}
