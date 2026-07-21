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
  const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
  if (!read.success) {
    std::cerr << read.error_message << "\n";
    return 1;
  }
  adasdf::ExactSDFOracle oracle;
  if (!oracle.reset(read.mesh)) {
    std::cerr << "failed to initialize exact SDF oracle\n";
    return 1;
  }
  adasdf::ConstrainedAdaptiveOctreeOptions options;
  options.min_depth = 1;
  options.max_depth = 6;
  options.narrow_band_distance = 0.02;
  options.max_center_interpolation_residual = 0.01;
  options.max_overlapping_triangles = 2;
  options.max_normal_complexity = 0.05;
  adasdf::ConstrainedAdaptiveOctreeStats stats;
  const adasdf::ConstrainedAdaptiveOctree tree =
      adasdf::ConstrainedAdaptiveOctreeBuilder::build(
          read.mesh, &oracle, options, &stats);
  if (!stats.success || tree.root_id != 0 || tree.nodes.empty() ||
      stats.leaf_count == 0 || stats.max_depth_used != options.max_depth) {
    std::cerr << "adaptive octree did not build to the expected depth\n";
    return 1;
  }
  if (stats.unique_exact_node_count == 0 ||
      stats.unique_exact_node_count >= stats.uniform_finest_grid_node_count) {
    std::cerr << "adaptive exact node count did not beat the uniform grid: "
              << stats.unique_exact_node_count << " vs "
              << stats.uniform_finest_grid_node_count << "\n";
    return 1;
  }
  if (stats.bvh_node_visits == 0 || stats.triangle_tests == 0 ||
      stats.triangle_refinement_count == 0) {
    std::cerr << "adaptive octree evidence counters are incomplete\n";
    return 1;
  }
  for (const adasdf::ConstrainedAdaptiveOctreeNode& node : tree.nodes) {
    if (node.node_id < 0 || node.level > options.max_depth) {
      std::cerr << "invalid deterministic node metadata\n";
      return 1;
    }
    if (!node.is_leaf) {
      for (const int child_id : node.child_ids) {
        if (child_id < 0 ||
            tree.nodes[static_cast<std::size_t>(child_id)].parent_id !=
                node.node_id) {
          std::cerr << "octree parent/child relationship is broken\n";
          return 1;
        }
      }
    }
  }
  adasdf::ExactSDFOracle repeat_oracle;
  repeat_oracle.reset(read.mesh);
  adasdf::ConstrainedAdaptiveOctreeStats repeat_stats;
  const auto repeat = adasdf::ConstrainedAdaptiveOctreeBuilder::build(
      read.mesh, &repeat_oracle, options, &repeat_stats);
  if (repeat.nodes.size() != tree.nodes.size() ||
      repeat.leafNodeIds() != tree.leafNodeIds()) {
    std::cerr << "adaptive octree build is not deterministic\n";
    return 1;
  }
  return 0;
}
