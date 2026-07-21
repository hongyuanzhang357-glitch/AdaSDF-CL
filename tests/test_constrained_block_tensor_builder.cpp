#include <adasdf/adasdf.h>

#include <filesystem>
#include <cmath>
#include <iostream>
#include <map>
#include <tuple>

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
    return 1;
  }
  adasdf::ConstrainedAdaptiveOctreeOptions tree_options;
  tree_options.max_depth = 3;
  tree_options.max_overlapping_triangles = 2;
  adasdf::ConstrainedAdaptiveOctreeStats tree_stats;
  const auto tree = adasdf::ConstrainedAdaptiveOctreeBuilder::build(
      read.mesh, &oracle, tree_options, &tree_stats);
  adasdf::ConstrainedBlockPartitionOptions block_options;
  block_options.target_leaf_cells_per_block = 32;
  block_options.max_leaf_cells_per_block = 128;
  block_options.max_decoded_block_bytes = 128 * 1024;
  adasdf::ConstrainedBlockPartitionStats block_stats;
  const auto blocks = adasdf::ConstrainedBlockPartitioner::partition(
      tree, block_options, &block_stats);
  if (!tree_stats.success || !block_stats.success || blocks.empty()) {
    return 1;
  }

  bool saw_interpolation = false;
  bool saw_exact = false;
  bool saw_promotion = false;
  std::map<std::tuple<long long, long long, long long>, double> shared_values;
  for (const auto& block : blocks) {
    adasdf::ConstrainedBlockTensorStats stats;
    auto tensor = adasdf::ConstrainedBlockTensorBuilder::build(
        tree, block, &oracle, {}, &stats);
    if (!stats.success || tensor.phi.size() != tensor.source.size() ||
        stats.tensor_node_count != tensor.phi.size()) {
      std::cerr << "tensor build failed\n";
      return 1;
    }
    saw_interpolation =
        saw_interpolation || stats.coarse_interpolated_node_count > 0;
    saw_exact = saw_exact || stats.exact_bvh_node_count > 0;
    adasdf::ConstrainedBlockTensorStats audit;
    if (!adasdf::ConstrainedBlockTensorBuilder::auditAndPromote(
            tree, &tensor, &oracle, 0.0, &audit)) {
      std::cerr << audit.error_message << "\n";
      return 1;
    }
    saw_promotion = saw_promotion || audit.promoted_exact_node_count > 0;
    for (int iz = 0; iz < block.tensor_nz; ++iz) {
      for (int iy = 0; iy < block.tensor_ny; ++iy) {
        for (int ix = 0; ix < block.tensor_nx; ++ix) {
          const double tx = static_cast<double>(ix) / (block.tensor_nx - 1);
          const double ty = static_cast<double>(iy) / (block.tensor_ny - 1);
          const double tz = static_cast<double>(iz) / (block.tensor_nz - 1);
          const double x = block.bounds.min.x +
              (block.bounds.max.x - block.bounds.min.x) * tx;
          const double y = block.bounds.min.y +
              (block.bounds.max.y - block.bounds.min.y) * ty;
          const double z = block.bounds.min.z +
              (block.bounds.max.z - block.bounds.min.z) * tz;
          const auto key = std::make_tuple(
              std::llround(x * 1.0e12),
              std::llround(y * 1.0e12),
              std::llround(z * 1.0e12));
          const std::size_t index = static_cast<std::size_t>(ix) +
              static_cast<std::size_t>(block.tensor_nx) *
                  (static_cast<std::size_t>(iy) +
                   static_cast<std::size_t>(block.tensor_ny) *
                       static_cast<std::size_t>(iz));
          const auto inserted = shared_values.emplace(key, tensor.phi[index]);
          if (!inserted.second &&
              std::abs(inserted.first->second - tensor.phi[index]) > 1.0e-12) {
            std::cerr << "shared block boundary node is discontinuous\n";
            return 1;
          }
        }
      }
    }
  }
  if (!saw_interpolation || !saw_exact || !saw_promotion) {
    std::cerr << "ExactBVH/CoarseInterpolated/promotion paths were not covered\n";
    return 1;
  }
  return 0;
}
