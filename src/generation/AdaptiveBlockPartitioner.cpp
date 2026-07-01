#include "adasdf/generation/AdaptiveBlockPartitioner.h"

#include <algorithm>
#include <stdexcept>

namespace adasdf {
namespace {

Vector3 spacingFor(const AABB& bounds, int n) {
  const double denom = static_cast<double>(std::max(1, n - 1));
  return {
      (bounds.max.x - bounds.min.x) / denom,
      (bounds.max.y - bounds.min.y) / denom,
      (bounds.max.z - bounds.min.z) / denom};
}

}  // namespace

AdaptiveSDFBlockSet AdaptiveBlockPartitioner::partition(
    const AdaptiveOctree& octree,
    const AdaptiveBlockPartitionOptions& options) {
  if (options.block_resolution < 2) {
    throw std::runtime_error("Adaptive block resolution must be at least 2.");
  }
  if (octree.root_id < 0 ||
      static_cast<std::size_t>(octree.root_id) >= octree.nodes.size()) {
    throw std::runtime_error("AdaptiveBlockPartitioner requires a valid octree.");
  }

  AdaptiveSDFBlockSet out;
  out.global_bounds = octree.nodes[static_cast<std::size_t>(octree.root_id)].bounds;
  const std::vector<int> leaves = octree.leafNodeIds();
  int next_block_id = 0;
  for (const int node_id : leaves) {
    const AdaptiveOctreeNode& node = octree.nodes[static_cast<std::size_t>(node_id)];
    if (!options.include_all_leaves && !node.near_surface) {
      continue;
    }
    AdaptiveSDFBlock block;
    block.block_id = next_block_id++;
    block.octree_node_id = node.id;
    block.level = node.level;
    block.bounds = node.bounds;
    block.nx = options.block_resolution;
    block.ny = options.block_resolution;
    block.nz = options.block_resolution;
    block.origin = node.bounds.min;
    block.spacing = spacingFor(node.bounds, options.block_resolution);
    block.near_surface =
        options.mark_near_surface_blocks ? node.near_surface : false;
    out.blocks.push_back(std::move(block));
  }
  return out;
}

}  // namespace adasdf
