#include "adasdf/generation/AdaptiveOctree.h"

#include <algorithm>

namespace adasdf {

AdaptiveOctreeNode::AdaptiveOctreeNode() {
  children.fill(-1);
}

std::vector<int> AdaptiveOctree::leafNodeIds() const {
  std::vector<int> leaves;
  leaves.reserve(nodes.size());
  for (const AdaptiveOctreeNode& node : nodes) {
    if (node.is_leaf) {
      leaves.push_back(node.id);
    }
  }
  return leaves;
}

std::size_t AdaptiveOctree::leafCount() const {
  return static_cast<std::size_t>(std::count_if(
      nodes.begin(),
      nodes.end(),
      [](const AdaptiveOctreeNode& node) { return node.is_leaf; }));
}

std::size_t AdaptiveOctree::nodeCount() const {
  return nodes.size();
}

int AdaptiveOctree::maxLevelUsed() const {
  int level = 0;
  for (const AdaptiveOctreeNode& node : nodes) {
    level = std::max(level, node.level);
  }
  return level;
}

}  // namespace adasdf
