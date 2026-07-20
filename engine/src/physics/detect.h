#pragma once

#include <vector>
#include <core/nodeRegistry.h>


std::vector<NodeHandle> detectBonePath(NodeHandle begin, NodeHandle end); // begin to end
std::vector<NodeHandle> detectBonePath(NodeHandle begin); // begin to end(auto detect)
