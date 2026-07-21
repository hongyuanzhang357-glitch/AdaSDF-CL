#include "adasdf/generation/ConstrainedAdaptiveOctree.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

Vector3 center(const AABB& box) {
  return 0.5 * (box.min + box.max);
}

double maxExtent(const AABB& box) {
  const Vector3 extent = box.max - box.min;
  return std::max({extent.x, extent.y, extent.z});
}

AABB paddedCube(const TriangleMesh& mesh, double padding_fraction) {
  const MeshAABB mesh_bounds = mesh.aabb();
  const Vector3 lo = toVector3(mesh_bounds.min);
  const Vector3 hi = toVector3(mesh_bounds.max);
  const Vector3 c = 0.5 * (lo + hi);
  const double side = std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z});
  const double half = 0.5 * side * (1.0 + 2.0 * std::max(0.0, padding_fraction));
  const double safe_half = std::max(half, 1.0e-12);
  return {c - Vector3{safe_half, safe_half, safe_half},
          c + Vector3{safe_half, safe_half, safe_half}, true};
}

AABB childBounds(const AABB& box, int child) {
  const Vector3 c = center(box);
  AABB out;
  out.valid = true;
  out.min = {
      (child & 1) != 0 ? c.x : box.min.x,
      (child & 2) != 0 ? c.y : box.min.y,
      (child & 4) != 0 ? c.z : box.min.z};
  out.max = {
      (child & 1) != 0 ? box.max.x : c.x,
      (child & 2) != 0 ? box.max.y : c.y,
      (child & 4) != 0 ? box.max.z : c.z};
  return out;
}

Vector3 corner(const AABB& box, int index) {
  return {
      (index & 1) != 0 ? box.max.x : box.min.x,
      (index & 2) != 0 ? box.max.y : box.min.y,
      (index & 4) != 0 ? box.max.z : box.min.z};
}

bool overlaps(const TriangleAABB& lhs, const AABB& rhs) {
  return lhs.valid && rhs.valid &&
      lhs.max.x >= rhs.min.x && lhs.min.x <= rhs.max.x &&
      lhs.max.y >= rhs.min.y && lhs.min.y <= rhs.max.y &&
      lhs.max.z >= rhs.min.z && lhs.min.z <= rhs.max.z;
}

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
  return {a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

double length(const Vector3& value) {
  return std::sqrt(dot(value, value));
}

struct OverlapMetrics {
  std::size_t triangle_count = 0;
  double normal_complexity = 0.0;
};

OverlapMetrics overlapMetrics(const TriangleBVH& bvh, const AABB& box) {
  OverlapMetrics out;
  if (!bvh.isValid() || bvh.mesh() == nullptr) {
    return out;
  }
  std::vector<int> stack{0};
  std::unordered_set<int> triangle_ids;
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    if (node_id < 0 || static_cast<std::size_t>(node_id) >= bvh.nodes().size()) {
      continue;
    }
    const TriangleBVHNode& node = bvh.nodes()[static_cast<std::size_t>(node_id)];
    if (!overlaps(node.bounds, box)) {
      continue;
    }
    if (!node.isLeaf()) {
      if (node.right >= 0) {
        stack.push_back(node.right);
      }
      if (node.left >= 0) {
        stack.push_back(node.left);
      }
      continue;
    }
    const std::size_t end = std::min(
        node.start + node.count, bvh.triangleIndices().size());
    for (std::size_t i = node.start; i < end; ++i) {
      const int triangle_id = bvh.triangleIndices()[i];
      const TriangleMesh& mesh = *bvh.mesh();
      if (triangle_id < 0 ||
          static_cast<std::size_t>(triangle_id) >= mesh.triangles.size()) {
        continue;
      }
      const TriangleAABB triangle_box =
          makeMeshTriangleAABB(mesh, mesh.triangles[static_cast<std::size_t>(triangle_id)]);
      if (overlaps(triangle_box, box)) {
        triangle_ids.insert(triangle_id);
      }
    }
  }
  out.triangle_count = triangle_ids.size();
  if (triangle_ids.empty()) {
    return out;
  }
  Vector3 normal_sum{};
  std::size_t normal_count = 0;
  const TriangleMesh& mesh = *bvh.mesh();
  for (const int triangle_id : triangle_ids) {
    const MeshTriangle& triangle =
        mesh.triangles[static_cast<std::size_t>(triangle_id)];
    const Vector3 a = toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v0)]);
    const Vector3 b = toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v1)]);
    const Vector3 c = toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v2)]);
    const Vector3 raw = cross(b - a, c - a);
    const double norm = length(raw);
    if (norm > 1.0e-15) {
      normal_sum = normal_sum + raw / norm;
      ++normal_count;
    }
  }
  if (normal_count > 0) {
    out.normal_complexity = std::clamp(
        1.0 - length(normal_sum) / static_cast<double>(normal_count),
        0.0,
        1.0);
  }
  return out;
}

std::size_t uniformNodeCount(int depth) {
  if (depth < 0 || depth >= 20) {
    return 0;
  }
  const std::size_t axis = (std::size_t{1} << depth) + 1;
  return axis * axis * axis;
}

struct BuildContext {
  const TriangleMesh& mesh;
  ExactSDFOracle& oracle;
  const ConstrainedAdaptiveOctreeOptions& options;
  ConstrainedAdaptiveOctree tree;
  ConstrainedAdaptiveOctreeStats stats;
  bool failed = false;

  int buildNode(const AABB& bounds, int level, int parent_id) {
    if (tree.nodes.size() >= options.max_node_count) {
      failed = true;
      stats.error_message = "adaptive octree exceeded max_node_count";
      return -1;
    }
    ConstrainedAdaptiveOctreeNode node;
    node.node_id = static_cast<int>(tree.nodes.size());
    node.parent_id = parent_id;
    node.level = level;
    node.bounds = bounds;
    node.min_abs_phi = std::numeric_limits<double>::infinity();
    bool has_positive = false;
    bool has_negative = false;
    for (int i = 0; i < 8; ++i) {
      const ExactSDFOracleSample sample = oracle.sample(corner(bounds, i));
      if (!sample.success) {
        failed = true;
        stats.error_message = "exact SDF sampling failed at octree corner";
        return -1;
      }
      node.corner_phi[static_cast<std::size_t>(i)] = sample.phi;
      node.min_abs_phi = std::min(node.min_abs_phi, std::abs(sample.phi));
      has_positive = has_positive || sample.phi > 0.0;
      has_negative = has_negative || sample.phi < 0.0;
    }
    for (double phi : node.corner_phi) {
      node.center_interpolated_phi += phi * 0.125;
    }
    node.sign_change = has_positive && has_negative;
    const double cell_extent = maxExtent(bounds);
    const OverlapMetrics overlap = overlapMetrics(oracle.bvh(), bounds);
    node.overlapping_triangle_count = overlap.triangle_count;
    node.normal_complexity = overlap.normal_complexity;
    node.triangle_overlap = overlap.triangle_count > 0;
    node.near_surface =
        node.min_abs_phi <= options.narrow_band_distance ||
        node.triangle_overlap;
    const bool needs_residual_probe =
        node.triangle_overlap || node.sign_change ||
        node.min_abs_phi <= options.narrow_band_distance;
    if (needs_residual_probe) {
      const ExactSDFOracleSample center_sample = oracle.sample(center(bounds));
      if (!center_sample.success) {
        failed = true;
        stats.error_message = "exact SDF sampling failed at octree center";
        return -1;
      }
      node.center_phi = center_sample.phi;
      node.min_abs_phi = std::min(node.min_abs_phi, std::abs(node.center_phi));
      node.interpolation_residual =
          std::abs(node.center_phi - node.center_interpolated_phi);
    } else {
      node.center_phi = node.center_interpolated_phi;
      node.interpolation_residual = 0.0;
    }
    node.geometry_complex =
        overlap.normal_complexity > options.max_normal_complexity;

    const int node_id = node.node_id;
    tree.nodes.push_back(node);
    ++stats.node_count_by_level[level];
    stats.max_depth_used = std::max(stats.max_depth_used, level);

    const bool residual_refine =
        node.interpolation_residual > options.max_center_interpolation_residual;
    const bool triangle_refine =
        node.overlapping_triangle_count > options.max_overlapping_triangles;
    const bool complexity_refine = node.geometry_complex;
    const bool minimum_depth = level < options.min_depth;
    const bool reached_depth = level >= options.max_depth;
    const bool reached_size =
        options.min_cell_size > 0.0 && cell_extent <= options.min_cell_size;
    const bool refine = !reached_depth && !reached_size &&
        (minimum_depth || node.sign_change || node.near_surface ||
         residual_refine || triangle_refine || complexity_refine);

    if (!refine) {
      ++stats.leaf_count;
      stats.near_surface_leaf_count += node.near_surface ? 1u : 0u;
      stats.sign_change_leaf_count += node.sign_change ? 1u : 0u;
      stats.triangle_overlap_leaf_count += node.triangle_overlap ? 1u : 0u;
      return node_id;
    }
    stats.residual_refinement_count += residual_refine ? 1u : 0u;
    stats.triangle_refinement_count += triangle_refine ? 1u : 0u;
    stats.complexity_refinement_count += complexity_refine ? 1u : 0u;
    tree.nodes[static_cast<std::size_t>(node_id)].is_leaf = false;
    for (int child = 0; child < 8; ++child) {
      const int child_id = buildNode(childBounds(bounds, child), level + 1, node_id);
      if (failed) {
        return node_id;
      }
      tree.nodes[static_cast<std::size_t>(node_id)]
          .child_ids[static_cast<std::size_t>(child)] = child_id;
    }
    return node_id;
  }
};

}  // namespace

std::vector<int> ConstrainedAdaptiveOctree::leafNodeIds() const {
  std::vector<int> ids;
  for (const ConstrainedAdaptiveOctreeNode& node : nodes) {
    if (node.is_leaf) {
      ids.push_back(node.node_id);
    }
  }
  return ids;
}

ConstrainedAdaptiveOctree ConstrainedAdaptiveOctreeBuilder::build(
    const TriangleMesh& mesh,
    ExactSDFOracle* oracle,
    const ConstrainedAdaptiveOctreeOptions& options,
    ConstrainedAdaptiveOctreeStats* stats_out) {
  const Clock::time_point begin = Clock::now();
  if (oracle == nullptr || !oracle->valid() || mesh.empty() ||
      options.min_depth < 0 || options.max_depth < options.min_depth ||
      options.max_depth >= 20 || options.max_node_count == 0 ||
      options.padding_fraction < 0.0 ||
      options.max_center_interpolation_residual < 0.0 ||
      options.narrow_band_distance < 0.0) {
    ConstrainedAdaptiveOctreeStats invalid_stats;
    invalid_stats.error_message = "invalid constrained adaptive octree input";
    invalid_stats.build_time_ms = std::chrono::duration<double, std::milli>(
        Clock::now() - begin).count();
    if (stats_out != nullptr) {
      *stats_out = invalid_stats;
    }
    return {};
  }

  BuildContext context{mesh, *oracle, options};
  const ExactSDFOracleStats before = oracle->stats();
  context.tree.bounds = paddedCube(mesh, options.padding_fraction);
  context.tree.root_id = context.buildNode(context.tree.bounds, 0, -1);
  const ExactSDFOracleStats after = oracle->stats();
  context.stats.node_count = context.tree.nodes.size();
  context.stats.exact_query_requests =
      after.exact_query_requests - before.exact_query_requests;
  context.stats.unique_exact_node_count =
      after.unique_exact_queries - before.unique_exact_queries;
  context.stats.exact_cache_hit_count = after.cache_hits - before.cache_hits;
  context.stats.bvh_node_visits =
      after.totalBVHNodeVisits() - before.totalBVHNodeVisits();
  context.stats.triangle_tests =
      after.totalTriangleTests() - before.totalTriangleTests();
  context.stats.uniform_finest_grid_node_count =
      uniformNodeCount(options.max_depth);
  context.stats.success = !context.failed && context.tree.root_id == 0;
  context.stats.build_time_ms = std::chrono::duration<double, std::milli>(
      Clock::now() - begin).count();
  if (stats_out != nullptr) {
    *stats_out = context.stats;
  }
  return context.tree;
}

}  // namespace adasdf
