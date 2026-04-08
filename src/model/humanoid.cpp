#include "model.h"
#include <unordered_map>


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

enum class BoneType {
	unknown,
	head,
	spine,
	hips,
	arm,
	leg,
	hand,
	foot
};

enum class Side {
	center,
	left,
	right
};

// 属性をidに付与
struct NodeInfo {
	int nodeId;
	BoneType type = BoneType::unknown;
	Side side;

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


void Humanoid::init(const std::vector<Node> nodes, const std::vector<Skin>& skins) {
	std::vector<NodeInfo> cands;
	// 属性をつける
	for (const auto& skin: skins) {
		for (const int& nodeID: skin.joints) {
			const std::vector<std::string>&& words = splitWords(nodes[nodeID].name);
			for (auto& w : words) {
				NodeInfo cand;
				if (auto it = wordMap.find(w); it != wordMap.end())
					cand.type = it->second;
				if (auto it = sideMap.find(w); it != sideMap.end())
					cand.side = it->second;
				cands.push_back(cand);
			}
		}
	}

	std::vector<NodeInfo> heads;
	std::vector<NodeInfo> leftArms;
	std::vector<NodeInfo> rightArms;
	std::vector<NodeInfo> spines;

	// 配列に分配
	for (const auto& node: cands) {
		switch (node.type) {
			case BoneType::head: {
				heads.push_back(node);
			} break;

			case BoneType::arm: {
				if (node.side == Side::left) leftArms.push_back(node);
				if (node.side == Side::right) rightArms.push_back(node);
			} break;

			case BoneType::spine: {
				spines.push_back(node);
			} break;
		}
	}
	
	printf("DEBUG: heads:%d arms_r:%d arms_l:%d spines:%d", heads.size(), rightArms.size(), leftArms.size(), spines.size());
}
