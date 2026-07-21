#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "adasdf/generation/ConstrainedAdaptiveOctree.h"

namespace adasdf {

struct ConstrainedBlockPartitionOptions {
  std::size_t min_leaf_cells_per_block = 4;
  std::size_t target_leaf_cells_per_block = 32;
  std::size_t max_leaf_cells_per_block = 128;
  int min_tensor_dimension = 3;
  int max_tensor_dimension = 33;
  int halo_nodes = 0;
  std::uint64_t max_decoded_block_bytes = 0;
};

struct ConstrainedSDFBlockPlan {
  int block_id = -1;
  int source_octree_node_id = -1;
  int level = 0;
  AABB bounds;
  std::vector<int> covered_leaf_node_ids;
  int finest_leaf_level = 0;
  int tensor_nx = 0;
  int tensor_ny = 0;
  int tensor_nz = 0;
  int halo_nodes = 0;
  bool near_surface = false;
  bool sign_change = false;
  bool resolution_clamped_for_memory = false;
  std::uint64_t tensor_output_bytes = 0;
  std::uint64_t decode_workspace_bytes = 0;
  std::uint64_t decoded_peak_bytes = 0;
};

struct ConstrainedBlockPartitionStats {
  bool success = false;
  std::string error_message;
  std::size_t block_count = 0;
  std::size_t covered_leaf_count = 0;
  std::size_t too_small_block_count = 0;
  std::size_t resolution_clamped_block_count = 0;
  std::uint64_t max_decoded_block_bytes = 0;
  bool every_leaf_covered_once = false;
  bool shared_boundary_grid_compatible = false;
};

class ConstrainedBlockPartitioner {
 public:
  static std::vector<ConstrainedSDFBlockPlan> partition(
      const ConstrainedAdaptiveOctree& tree,
      const ConstrainedBlockPartitionOptions& options,
      ConstrainedBlockPartitionStats* stats = nullptr);
};

}  // namespace adasdf
