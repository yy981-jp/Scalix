#include "model.h"
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <ranges>


std::vector<std::string> splitWords(const std::string& input) {
	std::vector<std::string> result;
	std::string current;

	auto push = [&]() {
		if (!current.empty()) {
			result.push_back(current);
			current.clear();
		}
	};

	for (size_t i = 0; i < input.size(); ++i) {
		char c = input[i];

		// 記号で区切る
		if (c == '_' || c == '.' || c == '-' || c == ':') {
			push();
			continue;
		}

		// 大文字で区切る（camelCase / PascalCase）
		if (std::isupper(c) && !current.empty()) {
			push();
		}

		current += std::tolower(c);
	}

	push();
	return result;
}

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

// 属性をidに付与
struct NodeInfo {
	int nodeId = -404;
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
/// @return nodeId
inline int select(const std::vector<NodeInfo> vec) {
	if (vec.empty()) return -404;
	return std::ranges::max_element(vec, {}, &NodeInfo::score) ->nodeId;
}

static inline void selectHelper(std::span<int> to, std::span<const std::vector<NodeInfo>> from, HBT target) {
	to[static_cast<size_t>(target)] = select( from[static_cast<size_t>(target)] );
}

void Humanoid::init(const std::vector<Node> nodes, const std::vector<Skin>& skins) {
	std::vector<NodeInfo> cands;
	// 属性をつける
	for (const auto& skin: skins) {
		for (const int& nodeId: skin.joints) {
			const std::vector<std::string> words = splitWords(nodes[nodeId].name);
			NodeInfo cand;
			cand.nodeId = nodeId;
			for (auto& w : words) {
				if (auto it = wordMap.find(w); it != wordMap.end())
					cand.type = it->second;
				if (auto it = sideMap.find(w); it != sideMap.end())
					cand.side = it->second;
				if (auto it = levelMap.find(w); it != levelMap.end())
					cand.level = it->second;

				// blacklist
				if (w == "support" || w == "ik")
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
			}

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
	for (const auto& spine: spines_) this->spines.push_back(spine.nodeId); // spineはすべて使用

	for (int i = 0; i < static_cast<size_t>(HBT::Count); i++) {
		selectHelper(bones, bones_init, static_cast<HBT>(i));
	}

}
