#include "adasdf/narrowband/BrickTensorDimensionPolicy.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

std::size_t nodeCount(int dim) {
  const int safe_dim = std::max(2, dim);
  return static_cast<std::size_t>(safe_dim) *
         static_cast<std::size_t>(safe_dim) *
         static_cast<std::size_t>(safe_dim);
}

std::size_t expandedBytes(int dim) {
  return nodeCount(dim) * sizeof(double);
}

int clampDim(int value, const NarrowBandBrickBuildOptions& options) {
  return std::clamp(
      value,
      std::max(2, options.brick_min_tensor_dim),
      std::max(2, options.brick_max_tensor_dim));
}

}  // namespace

BrickTensorDimensionDecision BrickTensorDimensionPolicy::decide(
    const CompressionBrick& brick,
    const NarrowBandBrickBuildOptions& options) {
  BrickTensorDimensionDecision out;
  const bool has_contact_demand = brick.contact_band_node_count > 0;
  const bool has_sampling_nodes = !brick.covered_sampling_node_ids.empty();
  int requested_dim = has_contact_demand
      ? options.brick_target_tensor_dim
      : options.brick_min_tensor_dim;
  if (!has_sampling_nodes) {
    requested_dim = options.brick_min_tensor_dim;
    out.reason = "empty_or_far_field";
  } else if (has_contact_demand) {
    out.reason = "contact_sampling_demand";
  } else {
    out.reason = "far_field_coarse";
  }

  int dim = clampDim(requested_dim, options);
  const std::size_t max_bytes = options.brick_max_expanded_kb > 0
      ? static_cast<std::size_t>(options.brick_max_expanded_kb) * 1024ull
      : 0u;
  if (max_bytes > 0 && expandedBytes(dim) > max_bytes) {
    while (dim > std::max(2, options.brick_min_tensor_dim) &&
           expandedBytes(dim) > max_bytes) {
      --dim;
    }
    out.warning_too_large = expandedBytes(dim) > max_bytes;
    out.should_split = out.warning_too_large || options.brick_split_on_memory;
    if (out.should_split) {
      out.reason = "expanded_memory_limit";
    }
  }

  if (has_contact_demand && dim < options.brick_target_tensor_dim) {
    out.warning_too_small = true;
    if (!out.should_split) {
      out.reason = "contact_demand_clamped";
    }
  }
  if (!has_contact_demand && options.brick_merge_small &&
      brick.covered_sampling_node_ids.size() <
          static_cast<std::size_t>(std::max(1, options.brick_min_compression_nodes))) {
    out.should_merge = true;
    out.reason = "small_far_field_brick";
  }

  out.tensor_nx = dim;
  out.tensor_ny = dim;
  out.tensor_nz = dim;
  out.tensor_node_count = nodeCount(dim);
  const double exact_ratio =
      has_contact_demand ? std::clamp(options.contact_exact_min_node_ratio, 0.0, 1.0) : 0.0;
  out.estimated_exact_source_node_count = static_cast<std::size_t>(
      std::ceil(exact_ratio * static_cast<double>(out.tensor_node_count)));
  out.estimated_interpolated_fill_node_count =
      out.tensor_node_count - out.estimated_exact_source_node_count;
  out.estimated_expanded_bytes = expandedBytes(dim);
  return out;
}

}  // namespace adasdf
