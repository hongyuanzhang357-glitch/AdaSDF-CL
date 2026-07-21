#include "adasdf/generation/AdaptiveTreeStats.h"

#include <algorithm>
#include <limits>
#include <map>

namespace adasdf {
namespace {

std::size_t saturatingMultiply(std::size_t lhs, std::size_t rhs) {
  if (lhs == 0 || rhs == 0) {
    return 0;
  }
  const std::size_t max_value = std::numeric_limits<std::size_t>::max();
  if (lhs > max_value / rhs) {
    return max_value;
  }
  return lhs * rhs;
}

std::size_t blockLogicalNodeCount(const AdaptiveSDFBlock& block) {
  if (block.nx <= 0 || block.ny <= 0 || block.nz <= 0) {
    return 0;
  }
  return saturatingMultiply(
      saturatingMultiply(
          static_cast<std::size_t>(block.nx),
          static_cast<std::size_t>(block.ny)),
      static_cast<std::size_t>(block.nz));
}

const AdaptiveTreeBlockSamplingStats* findSamplingStats(
    const std::vector<AdaptiveTreeBlockSamplingStats>& sampling_stats,
    int block_id) {
  for (const AdaptiveTreeBlockSamplingStats& item : sampling_stats) {
    if (item.block_id == block_id) {
      return &item;
    }
  }
  return nullptr;
}

void finalizeLevelAverages(AdaptiveTreeLevelStats* level) {
  if (level == nullptr || level->leaf_block_count == 0) {
    return;
  }
  const double n = static_cast<double>(level->leaf_block_count);
  level->avg_block_size_x /= n;
  level->avg_block_size_y /= n;
  level->avg_block_size_z /= n;
  level->avg_cell_size_x /= n;
  level->avg_cell_size_y /= n;
  level->avg_cell_size_z /= n;
}

}  // namespace

std::size_t adaptiveUniformLeafCountForLevel(int level) {
  if (level < 0) {
    return 0;
  }
  std::size_t count = 1;
  for (int i = 0; i < level; ++i) {
    count = saturatingMultiply(count, 8);
  }
  return count;
}

std::size_t adaptiveLogicalNodesPerBlock(int block_resolution) {
  if (block_resolution <= 0) {
    return 0;
  }
  const std::size_t n = static_cast<std::size_t>(block_resolution);
  return saturatingMultiply(saturatingMultiply(n, n), n);
}

AdaptiveTreeStats computeAdaptiveTreeStats(
    const AdaptiveOctree& octree,
    const AdaptiveSDFBlockSet& blocks,
    int block_resolution,
    int requested_max_level,
    const std::vector<AdaptiveTreeBlockSamplingStats>& sampling_stats) {
  AdaptiveTreeStats stats;
  stats.block_resolution = block_resolution;
  stats.max_level = requested_max_level;
  stats.max_level_used = octree.maxLevelUsed();
  stats.domain_bounds = blocks.global_bounds;
  stats.octree_node_count = octree.nodeCount();
  stats.leaf_block_count = blocks.blocks.size();

  std::map<int, AdaptiveTreeLevelStats> by_level;
  bool saw_leaf_level = false;
  int min_leaf_level = std::numeric_limits<int>::max();
  int max_leaf_level = std::numeric_limits<int>::min();

  for (const AdaptiveOctreeNode& node : octree.nodes) {
    AdaptiveTreeLevelStats& level = by_level[node.level];
    level.level = node.level;
    level.block_resolution = block_resolution;
    level.nodes_per_block = adaptiveLogicalNodesPerBlock(block_resolution);
    if (node.is_leaf) {
      saw_leaf_level = true;
      min_leaf_level = std::min(min_leaf_level, node.level);
      max_leaf_level = std::max(max_leaf_level, node.level);
    } else {
      ++level.internal_node_count;
      ++level.refined_node_count;
      ++stats.internal_node_count;
      ++stats.refined_node_count;
    }
  }

  for (const AdaptiveSDFBlock& block : blocks.blocks) {
    AdaptiveTreeLevelStats& level = by_level[block.level];
    level.level = block.level;
    level.block_resolution = block_resolution;
    level.nodes_per_block = adaptiveLogicalNodesPerBlock(block_resolution);
    ++level.leaf_block_count;
    if (block.near_surface) {
      ++stats.near_surface_leaf_block_count;
      ++level.near_surface_leaf_count;
    }

    const std::size_t logical = blockLogicalNodeCount(block);
    std::size_t exact = logical;
    std::size_t predicted = 0;
    std::size_t far_field = 0;
    if (const AdaptiveTreeBlockSamplingStats* sampled =
            findSamplingStats(sampling_stats, block.block_id)) {
      exact = sampled->exact_node_count;
      predicted = sampled->predicted_node_count;
      far_field = sampled->far_field_node_count;
      if (sampled->contact_band) {
        ++level.contact_band_leaf_count;
      }
      if (sampled->far_field) {
        ++level.far_field_leaf_count;
      }
    }

    level.logical_node_count += logical;
    level.exact_node_count += exact;
    level.predicted_node_count += predicted;
    level.far_field_node_count += far_field;

    stats.total_logical_node_count += logical;
    stats.exact_node_count += exact;
    stats.predicted_node_count += predicted;
    stats.far_field_node_count += far_field;

    const Vector3 block_size = block.bounds.max - block.bounds.min;
    level.avg_block_size_x += block_size.x;
    level.avg_block_size_y += block_size.y;
    level.avg_block_size_z += block_size.z;
    level.avg_cell_size_x += block.spacing.x;
    level.avg_cell_size_y += block.spacing.y;
    level.avg_cell_size_z += block.spacing.z;
  }

  stats.min_level = saw_leaf_level ? min_leaf_level : 0;
  stats.max_level_used = saw_leaf_level ? max_leaf_level : stats.max_level_used;
  stats.mixed_level_present = min_leaf_level != max_leaf_level;

  stats.uniform_max_level_leaf_count =
      adaptiveUniformLeafCountForLevel(requested_max_level);
  stats.uniform_max_level_logical_node_count = saturatingMultiply(
      stats.uniform_max_level_leaf_count,
      adaptiveLogicalNodesPerBlock(block_resolution));
  if (stats.uniform_max_level_logical_node_count > 0) {
    stats.sparsity_ratio_vs_uniform_max_level =
        static_cast<double>(stats.total_logical_node_count) /
        static_cast<double>(stats.uniform_max_level_logical_node_count);
  }
  stats.appears_uniform_max_level =
      !stats.mixed_level_present &&
      stats.max_level_used == requested_max_level &&
      stats.leaf_block_count == stats.uniform_max_level_leaf_count;

  stats.levels.reserve(by_level.size());
  for (auto& item : by_level) {
    finalizeLevelAverages(&item.second);
    stats.levels.push_back(item.second);
  }
  return stats;
}

}  // namespace adasdf
