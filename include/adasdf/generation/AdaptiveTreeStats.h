#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/generation/AdaptiveOctree.h"

namespace adasdf {

struct AdaptiveTreeBlockSamplingStats {
  int block_id = -1;
  int level = 0;

  bool contact_band = false;
  bool far_field = false;

  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t far_field_node_count = 0;
  std::size_t logical_node_count = 0;
};

struct AdaptiveTreeLevelStats {
  int level = 0;

  std::size_t internal_node_count = 0;
  std::size_t refined_node_count = 0;
  std::size_t leaf_block_count = 0;
  std::size_t far_field_leaf_count = 0;
  std::size_t contact_band_leaf_count = 0;
  std::size_t near_surface_leaf_count = 0;
  std::size_t transition_leaf_count = 0;

  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t far_field_node_count = 0;
  std::size_t logical_node_count = 0;

  double avg_block_size_x = 0.0;
  double avg_block_size_y = 0.0;
  double avg_block_size_z = 0.0;

  double avg_cell_size_x = 0.0;
  double avg_cell_size_y = 0.0;
  double avg_cell_size_z = 0.0;

  int block_resolution = 0;
  std::size_t nodes_per_block = 0;
};

struct AdaptiveTreeStats {
  int min_level = 0;
  int max_level = 0;
  int max_level_used = 0;
  int block_resolution = 0;

  AABB domain_bounds;

  std::size_t octree_node_count = 0;
  std::size_t internal_node_count = 0;
  std::size_t refined_node_count = 0;
  std::size_t leaf_block_count = 0;
  std::size_t near_surface_leaf_block_count = 0;

  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t far_field_node_count = 0;
  std::size_t total_logical_node_count = 0;

  std::size_t uniform_max_level_leaf_count = 0;
  std::size_t uniform_max_level_logical_node_count = 0;
  double sparsity_ratio_vs_uniform_max_level = 0.0;

  bool appears_uniform_max_level = false;
  bool mixed_level_present = false;

  std::vector<AdaptiveTreeLevelStats> levels;
  std::vector<std::string> warnings;
};

std::size_t adaptiveUniformLeafCountForLevel(int level);
std::size_t adaptiveLogicalNodesPerBlock(int block_resolution);

AdaptiveTreeStats computeAdaptiveTreeStats(
    const AdaptiveOctree& octree,
    const AdaptiveSDFBlockSet& blocks,
    int block_resolution,
    int requested_max_level,
    const std::vector<AdaptiveTreeBlockSamplingStats>& sampling_stats = {});

}  // namespace adasdf
