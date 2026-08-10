#pragma once
#include "def/node.h"
#include <cstdint>
#include <array>

#include <model/model.h>


enum class HBT {
	hips,

	spine,
	chest,
	upperChest,
	neck,
	head,

	leftShoulder,
	leftUpperArm,
	leftLowerArm,
	leftHand,

	rightShoulder,
	rightUpperArm,
	rightLowerArm,
	rightHand,

	leftUpperLeg,
	leftLowerLeg,
	leftFoot,
	leftToes,

	rightUpperLeg,
	rightLowerLeg,
	rightFoot,
	rightToes,

	Count
};

constexpr uint64_t HBTFlag(HBT bone) {
	return 1u << static_cast<uint64_t>(bone);
}

/// @brief 骨格
struct Humanoid {
	void init(const Model& model);

	// node handle
	std::array<NodeHandle,static_cast<size_t>(HBT::Count)> bones;
	std::vector<NodeHandle> spines;

	// std::vector<NodeHandle> nodes;

	uint64_t boneFlag = 0;
	
	bool has(HBT bone) {
		return boneFlag & HBTFlag(bone);
	}

	static constexpr std::array<HBT, (size_t)HBT::Count> parentTable = {
		// hips
		HBT::Count,
		// spine
		HBT::hips,
		// chest
		HBT::spine,
		// upperChest
		HBT::chest,
		// neck
		HBT::upperChest,
		// head
		HBT::neck,
		// leftShoulder
		HBT::upperChest,
		// leftUpperArm
		HBT::leftShoulder,
		// leftLowerArm
		HBT::leftUpperArm,
		// leftHand
		HBT::leftLowerArm,
		// rightShoulder
		HBT::upperChest,
		// rightUpperArm
		HBT::rightShoulder,
		// rightLowerArm
		HBT::rightUpperArm,
		// rightHand
		HBT::rightLowerArm,
		// leftUpperLeg
		HBT::hips,
		// leftLowerLeg
		HBT::leftUpperLeg,
		// leftFoot
		HBT::leftLowerLeg,
		// leftToes
		HBT::leftFoot,
		// rightUpperLeg
		HBT::hips,
		// rightLowerLeg
		HBT::rightUpperLeg,
		// rightFoot
		HBT::rightLowerLeg,
		// rightToes
		HBT::rightFoot,
	};

};
