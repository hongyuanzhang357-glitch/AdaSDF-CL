#include "adasdf/generation/ConstrainedBlockTensorBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace adasdf {
namespace {

struct GridKey {
  std::int64_t x = 0;
  std::int64_t y = 0;
  std::int64_t z = 0;

  bool operator==(const GridKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

struct GridKeyHash {
  std::size_t operator()(const GridKey& value) const noexcept {
    std::size_t seed = static_cast<std::size_t>(value.x);
    seed ^= static_cast<std::size_t>(value.y) + 0x9e3779b9u +
        (seed << 6) + (seed >> 2);
    seed ^= static_cast<std::size_t>(value.z) + 0x9e3779b9u +
        (seed << 6) + (seed >> 2);
    return seed;
  }
};

double rootSide(const ConstrainedAdaptiveOctree& tree) {
  return tree.bounds.max.x - tree.bounds.min.x;
}

int maxLevel(const ConstrainedAdaptiveOctree& tree) {
  int level = 0;
  for (const ConstrainedAdaptiveOctreeNode& node : tree.nodes) {
    level = std::max(level, node.level);
  }
  return level;
}

GridKey gridKey(
    const ConstrainedAdaptiveOctree& tree,
    const Vector3& point,
    int finest_level) {
  const double spacing = rootSide(tree) /
      static_cast<double>(std::uint64_t{1} << finest_level);
  return {
      static_cast<std::int64_t>(std::llround(
          (point.x - tree.bounds.min.x) / spacing)),
      static_cast<std::int64_t>(std::llround(
          (point.y - tree.bounds.min.y) / spacing)),
      static_cast<std::int64_t>(std::llround(
          (point.z - tree.bounds.min.z) / spacing))};
}

Vector3 corner(const AABB& box, int index) {
  return {
      (index & 1) != 0 ? box.max.x : box.min.x,
      (index & 2) != 0 ? box.max.y : box.min.y,
      (index & 4) != 0 ? box.max.z : box.min.z};
}

Vector3 tensorPoint(
    const ConstrainedSDFBlockPlan& block,
    int ix,
    int iy,
    int iz) {
  const double tx = block.tensor_nx > 1
      ? static_cast<double>(ix) / static_cast<double>(block.tensor_nx - 1)
      : 0.0;
  const double ty = block.tensor_ny > 1
      ? static_cast<double>(iy) / static_cast<double>(block.tensor_ny - 1)
      : 0.0;
  const double tz = block.tensor_nz > 1
      ? static_cast<double>(iz) / static_cast<double>(block.tensor_nz - 1)
      : 0.0;
  return {
      block.bounds.min.x + (block.bounds.max.x - block.bounds.min.x) * tx,
      block.bounds.min.y + (block.bounds.max.y - block.bounds.min.y) * ty,
      block.bounds.min.z + (block.bounds.max.z - block.bounds.min.z) * tz};
}

std::size_t valueIndex(
    int ix,
    int iy,
    int iz,
    int nx,
    int ny) {
  return static_cast<std::size_t>(ix) +
      static_cast<std::size_t>(nx) *
          (static_cast<std::size_t>(iy) +
           static_cast<std::size_t>(ny) * static_cast<std::size_t>(iz));
}

bool contains(const AABB& box, const Vector3& point) {
  const double eps = 1.0e-12;
  return box.valid && point.x >= box.min.x - eps &&
      point.x <= box.max.x + eps && point.y >= box.min.y - eps &&
      point.y <= box.max.y + eps && point.z >= box.min.z - eps &&
      point.z <= box.max.z + eps;
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double interpolateLeaf(
    const ConstrainedAdaptiveOctreeNode& leaf,
    const Vector3& point) {
  const Vector3 extent = leaf.bounds.max - leaf.bounds.min;
  const double tx = extent.x > 0.0
      ? std::clamp((point.x - leaf.bounds.min.x) / extent.x, 0.0, 1.0)
      : 0.0;
  const double ty = extent.y > 0.0
      ? std::clamp((point.y - leaf.bounds.min.y) / extent.y, 0.0, 1.0)
      : 0.0;
  const double tz = extent.z > 0.0
      ? std::clamp((point.z - leaf.bounds.min.z) / extent.z, 0.0, 1.0)
      : 0.0;
  const double c00 = lerp(leaf.corner_phi[0], leaf.corner_phi[1], tx);
  const double c10 = lerp(leaf.corner_phi[2], leaf.corner_phi[3], tx);
  const double c01 = lerp(leaf.corner_phi[4], leaf.corner_phi[5], tx);
  const double c11 = lerp(leaf.corner_phi[6], leaf.corner_phi[7], tx);
  return lerp(lerp(c00, c10, ty), lerp(c01, c11, ty), tz);
}

const ConstrainedAdaptiveOctreeNode* containingLeaf(
    const ConstrainedAdaptiveOctree& tree,
    const ConstrainedSDFBlockPlan& block,
    const Vector3& point) {
  const ConstrainedAdaptiveOctreeNode* selected = nullptr;
  for (const int leaf_id : block.covered_leaf_node_ids) {
    const ConstrainedAdaptiveOctreeNode& leaf =
        tree.nodes[static_cast<std::size_t>(leaf_id)];
    if (!contains(leaf.bounds, point)) {
      continue;
    }
    if (selected == nullptr || leaf.level > selected->level ||
        (leaf.level == selected->level && leaf.node_id < selected->node_id)) {
      selected = &leaf;
    }
  }
  return selected;
}

std::unordered_map<GridKey, double, GridKeyHash> exactCornerMap(
    const ConstrainedAdaptiveOctree& tree,
    int finest_level) {
  std::unordered_map<GridKey, double, GridKeyHash> values;
  for (const ConstrainedAdaptiveOctreeNode& node : tree.nodes) {
    for (int i = 0; i < 8; ++i) {
      values.emplace(
          gridKey(tree, corner(node.bounds, i), finest_level),
          node.corner_phi[static_cast<std::size_t>(i)]);
    }
  }
  return values;
}

bool sameSign(double lhs, double rhs) {
  return (lhs < 0.0) == (rhs < 0.0) || lhs == 0.0 || rhs == 0.0;
}

}  // namespace

const char* toString(SDFTensorNodeSource source) {
  switch (source) {
    case SDFTensorNodeSource::ExactBVH:
      return "ExactBVH";
    case SDFTensorNodeSource::CoarseInterpolated:
      return "CoarseInterpolated";
    case SDFTensorNodeSource::PromotedExactBVH:
      return "PromotedExactBVH";
  }
  return "CoarseInterpolated";
}

ConstrainedBlockTensor ConstrainedBlockTensorBuilder::build(
    const ConstrainedAdaptiveOctree& tree,
    const ConstrainedSDFBlockPlan& block,
    ExactSDFOracle* oracle,
    const ConstrainedBlockTensorOptions& options,
    ConstrainedBlockTensorStats* stats_out) {
  ConstrainedBlockTensorStats stats;
  ConstrainedBlockTensor tensor;
  tensor.plan = block;
  if (oracle == nullptr || !oracle->valid() || tree.nodes.empty() ||
      block.tensor_nx < 2 || block.tensor_ny < 2 || block.tensor_nz < 2) {
    stats.error_message = "invalid constrained block tensor input";
    if (stats_out != nullptr) {
      *stats_out = stats;
    }
    return tensor;
  }
  const int finest_level = maxLevel(tree);
  const auto exact_values = exactCornerMap(tree, finest_level);
  const std::size_t count = static_cast<std::size_t>(block.tensor_nx) *
      static_cast<std::size_t>(block.tensor_ny) *
      static_cast<std::size_t>(block.tensor_nz);
  tensor.phi.resize(count);
  tensor.source.resize(count, SDFTensorNodeSource::CoarseInterpolated);
  for (int iz = 0; iz < block.tensor_nz; ++iz) {
    for (int iy = 0; iy < block.tensor_ny; ++iy) {
      for (int ix = 0; ix < block.tensor_nx; ++ix) {
        const std::size_t index = valueIndex(
            ix, iy, iz, block.tensor_nx, block.tensor_ny);
        const Vector3 point = tensorPoint(block, ix, iy, iz);
        const GridKey key = gridKey(tree, point, finest_level);
        const auto exact = exact_values.find(key);
        if (exact != exact_values.end()) {
          tensor.phi[index] = exact->second;
          tensor.source[index] = SDFTensorNodeSource::ExactBVH;
          ++stats.exact_bvh_node_count;
          continue;
        }
        const auto interpolation_begin = std::chrono::steady_clock::now();
        const ConstrainedAdaptiveOctreeNode* leaf =
            containingLeaf(tree, block, point);
        if (leaf == nullptr) {
          stats.error_message = "tensor node is not covered by an octree leaf";
          if (stats_out != nullptr) {
            *stats_out = stats;
          }
          return {};
        }
        tensor.phi[index] = interpolateLeaf(*leaf, point);
        stats.interpolation_time_ms +=
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - interpolation_begin)
                .count();
        ++stats.coarse_interpolated_node_count;
        if (options.promote_near_zero_distance > 0.0 &&
            std::abs(tensor.phi[index]) <= options.promote_near_zero_distance) {
          const ExactSDFOracleSample promoted = oracle->sample(point);
          if (!promoted.success) {
            stats.error_message = "near-zero exact-node promotion failed";
            if (stats_out != nullptr) {
              *stats_out = stats;
            }
            return {};
          }
          tensor.phi[index] = promoted.phi;
          tensor.source[index] = SDFTensorNodeSource::PromotedExactBVH;
          --stats.coarse_interpolated_node_count;
          ++stats.promoted_exact_node_count;
        }
      }
    }
  }
  stats.tensor_node_count = count;
  stats.success = true;
  if (stats_out != nullptr) {
    *stats_out = stats;
  }
  return tensor;
}

bool ConstrainedBlockTensorBuilder::auditAndPromote(
    const ConstrainedAdaptiveOctree&,
    ConstrainedBlockTensor* tensor,
    ExactSDFOracle* oracle,
    double max_abs_error,
    ConstrainedBlockTensorStats* stats_out) {
  ConstrainedBlockTensorStats stats;
  if (tensor == nullptr || oracle == nullptr || !oracle->valid() ||
      !(max_abs_error >= 0.0) || !std::isfinite(max_abs_error) ||
      tensor->phi.size() != tensor->source.size()) {
    stats.error_message = "invalid interpolation audit input";
    if (stats_out != nullptr) {
      *stats_out = stats;
    }
    return false;
  }
  stats.tensor_node_count = tensor->phi.size();
  for (int iz = 0; iz < tensor->plan.tensor_nz; ++iz) {
    for (int iy = 0; iy < tensor->plan.tensor_ny; ++iy) {
      for (int ix = 0; ix < tensor->plan.tensor_nx; ++ix) {
        const std::size_t index = valueIndex(
            ix, iy, iz, tensor->plan.tensor_nx, tensor->plan.tensor_ny);
        if (tensor->source[index] != SDFTensorNodeSource::CoarseInterpolated) {
          stats.exact_bvh_node_count +=
              tensor->source[index] == SDFTensorNodeSource::ExactBVH ? 1u : 0u;
          stats.promoted_exact_node_count +=
              tensor->source[index] == SDFTensorNodeSource::PromotedExactBVH
              ? 1u
              : 0u;
          continue;
        }
        ++stats.promotion_candidate_count;
        const Vector3 point = tensorPoint(tensor->plan, ix, iy, iz);
        const ExactSDFOracleSample exact = oracle->sample(point);
        if (!exact.success) {
          stats.error_message = "exact interpolation audit query failed";
          if (stats_out != nullptr) {
            *stats_out = stats;
          }
          return false;
        }
        const double error = std::abs(exact.phi - tensor->phi[index]);
        stats.max_interpolation_abs_error =
            std::max(stats.max_interpolation_abs_error, error);
        const bool sign_mismatch = !sameSign(exact.phi, tensor->phi[index]);
        stats.sign_mismatch_count += sign_mismatch ? 1u : 0u;
        if (error > max_abs_error || sign_mismatch) {
          tensor->phi[index] = exact.phi;
          tensor->source[index] = SDFTensorNodeSource::PromotedExactBVH;
          ++stats.promoted_exact_node_count;
        } else {
          ++stats.coarse_interpolated_node_count;
        }
      }
    }
  }
  stats.success = true;
  if (stats_out != nullptr) {
    *stats_out = stats;
  }
  return true;
}

}  // namespace adasdf
