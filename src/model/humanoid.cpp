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
	spine,
	hips,
	arm,
	leg,
	hand,
	foot
};

enum class Side: uint8_t {
	unknown,
	center,
	left,
	right
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
	return std::ranges::max_element(vec, {}, &NodeInfo::score) ->nodeId;
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
				// std::cout << w << " ";
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
			// printf("\n");
		}
	}

	std::vector<NodeInfo> heads;
	std::vector<NodeInfo> arms_left_up;
	std::vector<NodeInfo> arms_left_low;
	std::vector<NodeInfo> arms_right_up;
	std::vector<NodeInfo> arms_right_low;
	std::vector<NodeInfo> spines_;

	// 配列に分配
	for (const auto& node: cands) {
		switch (node.type) {
			case BoneType::head: {
				heads.push_back(node);
			} break;

			case BoneType::arm: {
				if (node.side == Side::left) {
					if (node.level == Level::upper) arms_left_up.push_back(node);
					if (node.level == Level::lower) arms_left_low.push_back(node);
				}
				if (node.side == Side::right) {
					if (node.level == Level::upper) arms_right_up.push_back(node);
					if (node.level == Level::lower) arms_right_low.push_back(node);
				}
			} break;

			case BoneType::spine: {
				spines_.push_back(node);
			} break;
		}
	}
	
	// printf("DEBUG: heads:%d arms_ur:%d arms_lr:%d arms_ul:%d arms_ll:%d spines:%d",
	// 	heads.size(), arms_right_up.size(), arms_right_low.size(), arms_left_up.size(), arms_left_low.size(), spines_.size());


	// 最適解を選択
	for (const auto& spine: spines_)
		this->spines.push_back(spine.nodeId);
	bones[static_cast<size_t>(HumanoidBoneType::head)] = select(heads);
	bones[static_cast<size_t>(HumanoidBoneType::arm_left_up)] = select(arms_left_up);
	bones[static_cast<size_t>(HumanoidBoneType::arm_left_low)] = select(arms_left_low);
	bones[static_cast<size_t>(HumanoidBoneType::arm_right_up)] = select(arms_right_up);
	bones[static_cast<size_t>(HumanoidBoneType::arm_right_low)] = select(arms_right_low);
	

	// exit(1000);
}
