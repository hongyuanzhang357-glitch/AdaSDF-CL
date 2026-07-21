#include "adasdf/generation/ConstrainedBlockPartitioner.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace adasdf {
namespace {

struct SubtreeSummary {
  std::size_t leaf_count = 0;
  int finest_leaf_level = 0;
  bool near_surface = false;
  bool sign_change = false;
  std::vector<int> leaf_ids;
};

std::uint64_t checkedCubeBytes(int dim) {
  if (dim < 2) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  const std::uint64_t n = static_cast<std::uint64_t>(dim);
  if (n > 1000000 || n * n > std::numeric_limits<std::uint64_t>::max() / n) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  const std::uint64_t nodes = n * n * n;
  return nodes > std::numeric_limits<std::uint64_t>::max() / sizeof(double)
      ? std::numeric_limits<std::uint64_t>::max()
      : nodes * sizeof(double);
}

std::uint64_t workspaceBytes(int dim, std::uint64_t output_bytes) {
  const std::uint64_t n = static_cast<std::uint64_t>(std::max(2, dim));
  const std::uint64_t slices = 3u * n * n * sizeof(double);
  if (output_bytes > std::numeric_limits<std::uint64_t>::max() - slices) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return output_bytes + slices;
}

std::uint64_t peakBytes(int dim) {
  const std::uint64_t output = checkedCubeBytes(dim);
  const std::uint64_t workspace = workspaceBytes(dim, output);
  if (workspace > std::numeric_limits<std::uint64_t>::max() - output) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return output + workspace;
}

int naturalDimension(
    const ConstrainedAdaptiveOctreeNode& node,
    const SubtreeSummary& summary,
    const ConstrainedBlockPartitionOptions& options) {
  const int delta = std::clamp(summary.finest_leaf_level - node.level, 0, 20);
  const std::uint64_t natural = (std::uint64_t{1} << delta) + 1u;
  const int requested = std::clamp(
      static_cast<int>(std::min<std::uint64_t>(
          natural, static_cast<std::uint64_t>(options.max_tensor_dimension))),
      options.min_tensor_dimension,
      options.max_tensor_dimension);
  int hierarchical = 2;
  for (int candidate = 3; candidate <= requested;
       candidate = 2 * (candidate - 1) + 1) {
    hierarchical = candidate;
    if (candidate > (std::numeric_limits<int>::max() - 1) / 2) {
      break;
    }
  }
  return std::max(2, hierarchical);
}

SubtreeSummary summarize(
    const ConstrainedAdaptiveOctree& tree,
    int node_id,
    std::vector<SubtreeSummary>* summaries) {
  const ConstrainedAdaptiveOctreeNode& node =
      tree.nodes[static_cast<std::size_t>(node_id)];
  SubtreeSummary out;
  out.near_surface = node.near_surface;
  out.sign_change = node.sign_change;
  out.finest_leaf_level = node.level;
  if (node.is_leaf) {
    out.leaf_count = 1;
    out.leaf_ids.push_back(node_id);
  } else {
    for (const int child_id : node.child_ids) {
      const SubtreeSummary child = summarize(tree, child_id, summaries);
      out.leaf_count += child.leaf_count;
      out.finest_leaf_level =
          std::max(out.finest_leaf_level, child.finest_leaf_level);
      out.near_surface = out.near_surface || child.near_surface;
      out.sign_change = out.sign_change || child.sign_change;
      out.leaf_ids.insert(
          out.leaf_ids.end(), child.leaf_ids.begin(), child.leaf_ids.end());
    }
  }
  (*summaries)[static_cast<std::size_t>(node_id)] = out;
  return out;
}

bool childrenWouldBeTooSmall(
    const ConstrainedAdaptiveOctreeNode& node,
    const std::vector<SubtreeSummary>& summaries,
    std::size_t minimum) {
  if (node.is_leaf) {
    return false;
  }
  for (const int child_id : node.child_ids) {
    if (summaries[static_cast<std::size_t>(child_id)].leaf_count >= minimum) {
      return false;
    }
  }
  return true;
}

struct PartitionContext {
  const ConstrainedAdaptiveOctree& tree;
  const ConstrainedBlockPartitionOptions& options;
  const std::vector<SubtreeSummary>& summaries;
  std::vector<ConstrainedSDFBlockPlan> blocks;
  std::string error;

  void emit(int node_id) {
    if (!error.empty()) {
      return;
    }
    const ConstrainedAdaptiveOctreeNode& node =
        tree.nodes[static_cast<std::size_t>(node_id)];
    const SubtreeSummary& summary = summaries[static_cast<std::size_t>(node_id)];
    int dim = naturalDimension(node, summary, options);
    const bool merge_small_children =
        summary.leaf_count <= options.max_leaf_cells_per_block &&
        childrenWouldBeTooSmall(
            node, summaries, options.min_leaf_cells_per_block);
    bool select = node.is_leaf ||
        (summary.leaf_count <= options.target_leaf_cells_per_block) ||
        merge_small_children;
    if (select && peakBytes(dim) > options.max_decoded_block_bytes &&
        !node.is_leaf) {
      select = false;
    }
    if (!select) {
      for (const int child_id : node.child_ids) {
        emit(child_id);
      }
      return;
    }

    const int natural_dim = dim;
    while (dim >= 2 && peakBytes(dim) > options.max_decoded_block_bytes) {
      dim = dim > 3 ? (dim - 1) / 2 + 1 : dim - 1;
    }
    if (dim < 2) {
      error = "max_decoded_block_bytes cannot hold the minimum block tensor";
      return;
    }
    ConstrainedSDFBlockPlan block;
    block.block_id = static_cast<int>(blocks.size());
    block.source_octree_node_id = node_id;
    block.level = node.level;
    block.bounds = node.bounds;
    block.covered_leaf_node_ids = summary.leaf_ids;
    block.finest_leaf_level = summary.finest_leaf_level;
    block.tensor_nx = dim;
    block.tensor_ny = dim;
    block.tensor_nz = dim;
    block.halo_nodes = options.halo_nodes;
    block.near_surface = summary.near_surface;
    block.sign_change = summary.sign_change;
    block.resolution_clamped_for_memory = dim < natural_dim;
    block.tensor_output_bytes = checkedCubeBytes(dim);
    block.decode_workspace_bytes = workspaceBytes(dim, block.tensor_output_bytes);
    block.decoded_peak_bytes =
        block.tensor_output_bytes + block.decode_workspace_bytes;
    blocks.push_back(std::move(block));
  }
};

bool validOptions(const ConstrainedBlockPartitionOptions& options) {
  return options.min_leaf_cells_per_block > 0 &&
      options.target_leaf_cells_per_block >= options.min_leaf_cells_per_block &&
      options.max_leaf_cells_per_block >= options.target_leaf_cells_per_block &&
      options.min_tensor_dimension >= 2 &&
      options.max_tensor_dimension >= options.min_tensor_dimension &&
      options.halo_nodes >= 0 && options.max_decoded_block_bytes > 0;
}

}  // namespace

std::vector<ConstrainedSDFBlockPlan> ConstrainedBlockPartitioner::partition(
    const ConstrainedAdaptiveOctree& tree,
    const ConstrainedBlockPartitionOptions& options,
    ConstrainedBlockPartitionStats* stats_out) {
  ConstrainedBlockPartitionStats stats;
  if (tree.root_id < 0 || tree.nodes.empty() || !validOptions(options)) {
    stats.error_message = "invalid constrained block partition input";
    if (stats_out != nullptr) {
      *stats_out = stats;
    }
    return {};
  }
  std::vector<SubtreeSummary> summaries(tree.nodes.size());
  summarize(tree, tree.root_id, &summaries);
  PartitionContext context{tree, options, summaries};
  context.emit(tree.root_id);
  if (!context.error.empty()) {
    stats.error_message = context.error;
    if (stats_out != nullptr) {
      *stats_out = stats;
    }
    return {};
  }

  std::unordered_map<int, std::size_t> coverage;
  for (const ConstrainedSDFBlockPlan& block : context.blocks) {
    stats.max_decoded_block_bytes =
        std::max(stats.max_decoded_block_bytes, block.decoded_peak_bytes);
    stats.resolution_clamped_block_count +=
        block.resolution_clamped_for_memory ? 1u : 0u;
    stats.too_small_block_count +=
        block.covered_leaf_node_ids.size() < options.min_leaf_cells_per_block
        ? 1u
        : 0u;
    for (const int leaf_id : block.covered_leaf_node_ids) {
      ++coverage[leaf_id];
    }
  }
  stats.block_count = context.blocks.size();
  stats.covered_leaf_count = coverage.size();
  stats.every_leaf_covered_once =
      coverage.size() == tree.leafNodeIds().size() &&
      std::all_of(coverage.begin(), coverage.end(), [](const auto& item) {
        return item.second == 1;
      });
  stats.shared_boundary_grid_compatible = true;
  stats.success = !context.blocks.empty() && stats.every_leaf_covered_once &&
      stats.max_decoded_block_bytes <= options.max_decoded_block_bytes;
  if (!stats.success) {
    stats.error_message = "block partition failed a coverage or memory invariant";
  }
  if (stats_out != nullptr) {
    *stats_out = stats;
  }
  return context.blocks;
}

}  // namespace adasdf
