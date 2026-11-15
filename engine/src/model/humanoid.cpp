#include <model/model.h>
#include <model/humanoid.h>
#include <util/fmutil.h>
#include <loader/data.h>
#include <loader/vrm.h>
#include <core/nodeRegistry.h>
#include <unordered_map>
#include <algorithm>

#include <y9inc/string.h>


template <typename... Targets>
bool has(const std::vector<std::string>& w, const Targets&... targets_arg) {
	const std::vector<std::string> targets = {targets_arg...};
	for (const auto& target: targets)
		for (const auto& s : w)
			if (s == target) return true;
	return false;
}

struct StringHash {
	using is_transparent = void;

	size_t operator()(std::string_view sv) const noexcept {
		return std::hash<std::string_view>{}(sv);
	}
};




enum class BoneType : uint8_t {
	unknown,

	head,
	neck,

	spine,
	chest,
	hips,

	shoulder,
	arm,

	hand,

	leg,
	foot,
	toes,
};

enum class Side: uint8_t {
	unknown,
	center,
	left,
	right,
};

enum class Level : uint8_t {
	unknown,

	upper,
	lower,

	middle,
};

struct NodeInfo {
	NodeHandle handle = NodeHandle{
		NodeEntryId(UINT32_MAX),
		NodeGen(0),
	};

	BoneType type = BoneType::unknown;
	Side side = Side::unknown;
	Level level = Level::unknown;

	int score = 0;
};


static const std::unordered_map<std::string, BoneType, StringHash, std::equal_to<>> wordMap = {
	{"head", BoneType::head},

	{"neck", BoneType::neck},

	{"spine", BoneType::spine},
	{"chest", BoneType::chest},

	{"hips", BoneType::hips},
	{"hip", BoneType::hips},
	{"pelvis", BoneType::hips},

	{"shoulder", BoneType::shoulder},
	{"clavicle", BoneType::shoulder},
	{"collar", BoneType::shoulder},

	{"arm", BoneType::arm},
	{"forearm", BoneType::arm},

	{"hand", BoneType::hand},
	{"wrist", BoneType::hand},

	{"leg", BoneType::leg},
	{"thigh", BoneType::leg},
	{"calf", BoneType::leg},
	{"shin", BoneType::leg},

	{"foot", BoneType::foot},
	{"ankle", BoneType::foot},

	{"toe", BoneType::toes},
	{"toes", BoneType::toes},
};

static const std::unordered_map<std::string, Side, StringHash, std::equal_to<>> sideMap = {
	{"l", Side::left},
	{"left", Side::left},

	{"r", Side::right},
	{"right", Side::right},
};

static const std::unordered_map<std::string, Level, StringHash, std::equal_to<>> levelMap = {
	{"upper", Level::upper},
	{"up", Level::upper},

	{"lower", Level::lower},
	{"low", Level::lower},

	{"mid", Level::middle},
	{"middle", Level::middle},
};

/// @brief 最適なnodeを選択
/// @param vec 
/// @return nodeHandle
inline NodeHandle select(std::span<const NodeInfo> vec) {
	if (vec.empty())
		return NodeHandle::invalid();

	return std::ranges::max_element(vec, {}, &NodeInfo::score)->handle;
}

static inline void selectHelper(std::span<NodeHandle> to,
  std::span<const std::vector<NodeInfo>> from, HBT target) {
	to[static_cast<size_t>(target)] = select(from[static_cast<size_t>(target)]);
}


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
