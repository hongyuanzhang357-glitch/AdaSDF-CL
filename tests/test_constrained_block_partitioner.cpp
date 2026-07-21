#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  const std::filesystem::path fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  adasdf::ExactSDFOracle oracle;
  if (!read.success || !oracle.reset(read.mesh)) {
    std::cerr << "failed to prepare cube fixture\n";
    return 1;
  }
  adasdf::ConstrainedAdaptiveOctreeOptions tree_options;
  tree_options.max_depth = 4;
  tree_options.max_overlapping_triangles = 2;
  adasdf::ConstrainedAdaptiveOctreeStats tree_stats;
  const auto tree = adasdf::ConstrainedAdaptiveOctreeBuilder::build(
      read.mesh, &oracle, tree_options, &tree_stats);
  if (!tree_stats.success) {
    std::cerr << tree_stats.error_message << "\n";
    return 1;
  }

  adasdf::ConstrainedBlockPartitionOptions options;
  options.min_leaf_cells_per_block = 4;
  options.target_leaf_cells_per_block = 32;
  options.max_leaf_cells_per_block = 128;
  options.max_decoded_block_bytes = 128 * 1024;
  adasdf::ConstrainedBlockPartitionStats stats;
  const auto blocks =
      adasdf::ConstrainedBlockPartitioner::partition(tree, options, &stats);
  if (!stats.success || blocks.empty() || !stats.every_leaf_covered_once ||
      !stats.shared_boundary_grid_compatible ||
      stats.max_decoded_block_bytes > options.max_decoded_block_bytes) {
    std::cerr << "constrained block partition invariants failed\n";
    return 1;
  }
  for (const auto& block : blocks) {
    if (block.covered_leaf_node_ids.empty() || block.tensor_nx < 2 ||
        block.decoded_peak_bytes > options.max_decoded_block_bytes) {
      std::cerr << "invalid block plan\n";
      return 1;
    }
  }

  adasdf::ConstrainedBlockPartitionOptions impossible = options;
  impossible.max_decoded_block_bytes = 64;
  adasdf::ConstrainedBlockPartitionStats impossible_stats;
  const auto no_blocks = adasdf::ConstrainedBlockPartitioner::partition(
      tree, impossible, &impossible_stats);
  if (impossible_stats.success || !no_blocks.empty() ||
      impossible_stats.error_message.empty()) {
    std::cerr << "impossible block memory budget was not rejected\n";
    return 1;
  }
  return 0;
}
