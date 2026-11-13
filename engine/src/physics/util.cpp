#include <physics/util.h>

#include <core/avatar.h>

#include <unordered_set>


std::vector<NodeHandle> detectBonePath(NodeHandle begin, NodeHandle end) {
	std::vector<NodeHandle> path;

	if (!nodeReg.isAlive(begin) || !nodeReg.isAlive(end))
		return path;

	if (nodeReg.getAvatar(begin) != nodeReg.getAvatar(end))
		return path;

	std::unordered_set<NodeHandle> visited;

	for (NodeHandle cur = end; nodeReg.isAlive(cur); ) {
		if (!visited.insert(cur).second)
			return {};

		path.push_back(cur);

		if (cur == begin)
			break;

		cur = nodeReg.get(cur).parent;
	}

	if (path.empty() || path.back() != begin)
		return {};

	std::reverse(path.begin(), path.end());
	return path;
}

std::vector<NodeHandle> detectBonePath(NodeHandle begin) {
	std::vector<NodeHandle> path;

	if (!nodeReg.isAlive(begin))
		return path;


	NodeHandle cur = begin;

	while (nodeReg.isAlive(cur)) {
		path.push_back(cur);

		const Node& node = nodeReg.get(cur);

		if (node.children.size() != 1)
			break;

		cur = node.children[0];
	}

	return path;
}
