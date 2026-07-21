#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <vector>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(
    int id,
    int level,
    const adasdf::AABB& bounds) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = id;
  block.octree_node_id = id;
  block.level = level;
  block.bounds = bounds;
  block.nx = 4;
  block.ny = 4;
  block.nz = 4;
  block.origin = bounds.min;
  block.spacing = {
      (bounds.max.x - bounds.min.x) / 3.0,
      (bounds.max.y - bounds.min.y) / 3.0,
      (bounds.max.z - bounds.min.z) / 3.0};
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveOctree octree;
    adasdf::AdaptiveOctreeNode root;
    root.id = 0;
    root.level = 0;
    root.is_leaf = false;
    octree.root_id = 0;
    octree.nodes.push_back(root);

    adasdf::AdaptiveOctreeNode leaf1;
    leaf1.id = 1;
    leaf1.parent_id = 0;
    leaf1.level = 1;
    leaf1.is_leaf = true;
    octree.nodes.push_back(leaf1);

    adasdf::AdaptiveOctreeNode internal;
    internal.id = 2;
    internal.parent_id = 0;
    internal.level = 1;
    internal.is_leaf = false;
    octree.nodes.push_back(internal);

    adasdf::AdaptiveOctreeNode leaf2;
    leaf2.id = 3;
    leaf2.parent_id = 2;
    leaf2.level = 2;
    leaf2.is_leaf = true;
    octree.nodes.push_back(leaf2);

    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.blocks.push_back(makeBlock(
        1,
        1,
        {{0.0, 0.0, 0.0}, {0.5, 1.0, 1.0}, true}));
    blocks.blocks.push_back(makeBlock(
        2,
        2,
        {{0.5, 0.0, 0.0}, {0.75, 0.5, 0.5}, true}));

    adasdf::AdaptiveTreeBlockSamplingStats sampled;
    sampled.block_id = 2;
    sampled.level = 2;
    sampled.exact_node_count = 8;
    sampled.predicted_node_count = 56;
    sampled.logical_node_count = 64;

    const auto stats = adasdf::computeAdaptiveTreeStats(
        octree,
        blocks,
        4,
        2,
        {sampled});
    if (!stats.mixed_level_present ||
        stats.appears_uniform_max_level ||
        stats.leaf_block_count != 2 ||
        stats.uniform_max_level_leaf_count != 64 ||
        stats.total_logical_node_count != 128 ||
        stats.exact_node_count != 72 ||
        stats.predicted_node_count != 56) {
      std::cerr << "adaptive tree stats mismatch\n";
      return 1;
    }
    std::cout << "adaptive tree stats passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_tree_stats failed: " << exc.what() << "\n";
    return 1;
  }
}
