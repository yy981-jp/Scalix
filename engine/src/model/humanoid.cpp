#include <model/model.h>
#include <util/fmutil.h>
#include <core/nodeRegistry.h>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <ranges>

#include <y9inc/string.h>


template <typename... Targets>
bool has(const std::vector<std::string>& w, const Targets&... targets_arg) {
	const std::vector<std::string> targets = {targets_arg...};
	for (const auto& target: targets)
		for (const auto& s : w)
			if (s == target) return true;
	return false;
}

enum class BoneType: uint8_t {
	unknown,
	head,
	neck,
	spine,
	hips,
	arm,
	leg,
	hand,
	foot,
};

enum class Side: uint8_t {
	unknown,
	center,
	left,
	right,
};

enum class Level: uint8_t {
	unknown,
	upper,
	lower,
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


static const std::unordered_map<std::string, BoneType> wordMap = {
	{"head", BoneType::head},
	{"neck", BoneType::neck},
	{"spine", BoneType::spine},
	{"hips", BoneType::hips},
	{"pelvis", BoneType::hips},

	{"arm", BoneType::arm},
	{"leg", BoneType::leg},

	{"hand", BoneType::hand},
	{"foot", BoneType::foot},
};

static const std::unordered_map<std::string, Side> sideMap = {
	{"l", Side::left},
	{"left", Side::left},

	{"r", Side::right},
	{"right", Side::right},
};

static const std::unordered_map<std::string, Level> levelMap = {
	{"upper", Level::upper},
	{"up", Level::upper},

	{"lower", Level::lower},
	{"low", Level::lower},
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

void Humanoid::init(const Model& model) {
	const auto& nodes = model.nodes;
	const auto& skins = model.skins;
	NodeHandle sampleHdl = model.nodeHandles[0];
	
	std::vector<NodeInfo> cands;
	// 属性をつける
	for (const auto& skin: skins) { // skins.size() == 1 であることも多い
		for (const NodeId& nodeId: skin.joints) {
			const std::vector<std::string> words = tokenizer(strsv().get( nodes[nodeId].name ));
			NodeInfo cand;
			cand.handle = nodeReg.find(sampleHdl,nodeId);
			for (auto& w : words) {
				if (auto it = wordMap.find(w); it != wordMap.end())
					cand.type = it->second;
				if (auto it = sideMap.find(w); it != sideMap.end())
					cand.side = it->second;
				if (auto it = levelMap.find(w); it != levelMap.end())
					cand.level = it->second;

				// blacklist
				if (is_or(w,"support","ik","twist","roll"))
					cand.score -= 100;
			}
			cands.push_back(cand);
		}
	}

	std::array<std::vector<NodeInfo>,static_cast<size_t>(HBT::Count)> bones_init;

	std::vector<NodeInfo> spines_;

	// 配列に分配
	for (const auto& node: cands) {
		switch (node.type) {
			case BoneType::head: {
				bones_init[static_cast<size_t>(HBT::head)].push_back(node);
			} break;

			case BoneType::neck: {
				bones_init[static_cast<size_t>(HBT::neck)].push_back(node);
			} break;

			case BoneType::arm: {
				if (node.side == Side::left) {
					if (node.level == Level::upper) bones_init[static_cast<size_t>(HBT::arm_left_up)].push_back(node);
					if (node.level == Level::lower) bones_init[static_cast<size_t>(HBT::arm_left_low)].push_back(node);
				}
				if (node.side == Side::right) {
					if (node.level == Level::upper) bones_init[static_cast<size_t>(HBT::arm_right_up)].push_back(node);
					if (node.level == Level::lower) bones_init[static_cast<size_t>(HBT::arm_right_low)].push_back(node);
				}
			} break;

			case BoneType::leg: {
				if (node.side == Side::left) {
					if (node.level == Level::upper) bones_init[static_cast<size_t>(HBT::leg_left_up)].push_back(node);
					if (node.level == Level::lower) bones_init[static_cast<size_t>(HBT::leg_left_low)].push_back(node);
				}
				if (node.side == Side::right) {
					if (node.level == Level::upper) bones_init[static_cast<size_t>(HBT::leg_right_up)].push_back(node);
					if (node.level == Level::lower) bones_init[static_cast<size_t>(HBT::leg_right_low)].push_back(node);
				}
			} break;

			case BoneType::spine: {
				spines_.push_back(node);
			} break;
		}
	}
	
	// 最適解を選択
	for (const auto& spine: spines_) this->spines.push_back(spine.handle); // spineはすべて使用

	for (int i = 0; i < static_cast<size_t>(HBT::Count); i++) {
		selectHelper(bones, bones_init, static_cast<HBT>(i));
		
		// TODO: fix: boneFlagに反映
		if (bones[i].isValid()) {
			boneFlag |= HBTFlag(static_cast<HBT>(i));
		}
	}



}
