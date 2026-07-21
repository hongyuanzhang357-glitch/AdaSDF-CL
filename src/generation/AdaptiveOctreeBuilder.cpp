#include "adasdf/generation/AdaptiveOctreeBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/acceleration/TriangleAABB.h"
#include "adasdf/mesh/MeshDiagnostics.h"

namespace adasdf {
namespace {

bool boxesOverlap(const TriangleAABB& tri_box, const AABB& cell) {
  return isValid(tri_box) && cell.valid &&
         tri_box.max.x >= cell.min.x && tri_box.min.x <= cell.max.x &&
         tri_box.max.y >= cell.min.y && tri_box.min.y <= cell.max.y &&
         tri_box.max.z >= cell.min.z && tri_box.min.z <= cell.max.z;
}

std::vector<TriangleAABB> makeTriangleAABBs(const TriangleMesh& mesh) {
  std::vector<TriangleAABB> boxes;
  boxes.reserve(mesh.triangles.size());
  for (const MeshTriangle& triangle : mesh.triangles) {
    boxes.push_back(makeMeshTriangleAABB(mesh, triangle));
  }
  return boxes;
}

bool triangleAABBOverlapsCell(
    const std::vector<TriangleAABB>& triangle_aabbs,
    const AABB& cell) {
  for (const TriangleAABB& triangle_aabb : triangle_aabbs) {
    if (boxesOverlap(triangle_aabb, cell)) {
      return true;
    }
  }
  return false;
}

double length(const Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

double signedPhi(
    const TriangleMesh& mesh,
    const Vector3& p,
    bool signed_distance,
    const BVHSDFSampler* sampler,
    const BVHSDFSamplerOptions& sampler_options) {
  BVHSDFSampleResult sample;
  if (sampler != nullptr && sampler->hasBVH()) {
    sample = BVHSDFSampler::sampleWithBVH(
        mesh,
        sampler->bvh(),
        p,
        sampler_options);
  } else {
    sample = BVHSDFSampler::sampleBruteForce(mesh, p, signed_distance);
  }
  if (!sample.success || !std::isfinite(sample.phi)) {
    return 0.0;
  }
  return sample.phi;
}

Vector3 midPoint(const Vector3& a, const Vector3& b) {
  return 0.5 * (a + b);
}

AABB paddedRoot(const TriangleMesh& mesh, double padding) {
  const MeshAABB mesh_box = mesh.aabb();
  Vector3 min_corner = toVector3(mesh_box.min);
  Vector3 max_corner = toVector3(mesh_box.max);
  const double pad = std::max(0.0, padding);
  min_corner = {
      min_corner.x - pad,
      min_corner.y - pad,
      min_corner.z - pad};
  max_corner = {
      max_corner.x + pad,
      max_corner.y + pad,
      max_corner.z + pad};
  const double min_extent = 1.0e-6;
  if (!(max_corner.x > min_corner.x)) {
    min_corner.x -= min_extent;
    max_corner.x += min_extent;
  }
  if (!(max_corner.y > min_corner.y)) {
    min_corner.y -= min_extent;
    max_corner.y += min_extent;
  }
  if (!(max_corner.z > min_corner.z)) {
    min_corner.z -= min_extent;
    max_corner.z += min_extent;
  }
  return {min_corner, max_corner, true};
}

std::vector<Vector3> cellSamples(const AABB& bounds) {
  const Vector3 c = midPoint(bounds.min, bounds.max);
  std::vector<Vector3> samples;
  samples.reserve(9);
  samples.push_back(c);
  for (int z = 0; z < 2; ++z) {
    for (int y = 0; y < 2; ++y) {
      for (int x = 0; x < 2; ++x) {
        samples.push_back({
            x == 0 ? bounds.min.x : bounds.max.x,
            y == 0 ? bounds.min.y : bounds.max.y,
            z == 0 ? bounds.min.z : bounds.max.z});
      }
    }
  }
  return samples;
}

void evaluateNode(
    const TriangleMesh& mesh,
    const AdaptiveOctreeBuildOptions& options,
    const std::vector<TriangleAABB>& triangle_aabbs,
    const BVHSDFSampler* sampler,
    const BVHSDFSamplerOptions& sampler_options,
    AdaptiveOctreeNode& node) {
  const Vector3 diag = node.bounds.max - node.bounds.min;
  node.cell_diagonal = length(diag);
  const Vector3 center = midPoint(node.bounds.min, node.bounds.max);
  node.center_phi = signedPhi(
      mesh,
      center,
      options.signed_distance,
      sampler,
      sampler_options);

  const std::vector<Vector3> samples = cellSamples(node.bounds);
  node.min_abs_sample_phi = std::numeric_limits<double>::infinity();
  node.max_abs_sample_phi = 0.0;
  bool saw_negative = false;
  bool saw_positive = false;
  bool saw_zero = false;
  for (const Vector3& sample : samples) {
    const double phi = signedPhi(
        mesh,
        sample,
        options.signed_distance,
        sampler,
        sampler_options);
    const double abs_phi = std::abs(phi);
    if (std::isfinite(abs_phi)) {
      node.min_abs_sample_phi = std::min(node.min_abs_sample_phi, abs_phi);
      node.max_abs_sample_phi = std::max(node.max_abs_sample_phi, abs_phi);
    }
    if (phi < 0.0) {
      saw_negative = true;
    } else if (phi > 0.0) {
      saw_positive = true;
    } else {
      saw_zero = true;
    }
  }
  if (!std::isfinite(node.min_abs_sample_phi)) {
    node.min_abs_sample_phi = 0.0;
  }
  node.contains_surface =
      saw_zero || (saw_negative && saw_positive) ||
      triangleAABBOverlapsCell(triangle_aabbs, node.bounds);
  const double near_band = std::max(0.0, options.surface_band_factor) *
                           std::max(node.cell_diagonal, 0.0);
  const double target_band =
      std::max(0.0, options.target_near_surface_error) *
      std::max(0.0, options.surface_band_factor);
  if (options.leaf_mode == AdaptiveLeafMode::Uniform) {
    node.near_surface =
        node.contains_surface ||
        (options.refine_near_surface &&
         (node.min_abs_sample_phi <= near_band ||
          node.min_abs_sample_phi <= target_band));
  } else {
    node.near_surface =
        node.contains_surface ||
        (options.refine_near_surface &&
         node.min_abs_sample_phi <= target_band);
  }
}

bool shouldRefine(
    const AdaptiveOctreeNode& node,
    const AdaptiveOctreeBuildOptions& options) {
  if (node.level < options.min_level) {
    return true;
  }
  if (node.level >= options.max_level) {
    return false;
  }
  if (options.leaf_mode == AdaptiveLeafMode::Uniform) {
    return true;
  }
  if (options.refine_sign_changes && node.contains_surface) {
    return true;
  }
  if (options.refine_near_surface && node.near_surface) {
    return true;
  }
  return false;
}

AABB childBounds(const AABB& parent, int child) {
  const Vector3 c = midPoint(parent.min, parent.max);
  const bool hi_x = (child & 1) != 0;
  const bool hi_y = (child & 2) != 0;
  const bool hi_z = (child & 4) != 0;
  return {
      {hi_x ? c.x : parent.min.x,
       hi_y ? c.y : parent.min.y,
       hi_z ? c.z : parent.min.z},
      {hi_x ? parent.max.x : c.x,
       hi_y ? parent.max.y : c.y,
       hi_z ? parent.max.z : c.z},
      true};
}

void addChildren(
    AdaptiveOctree& octree,
    int parent_id,
    const TriangleMesh& mesh,
    const AdaptiveOctreeBuildOptions& options,
    const std::vector<TriangleAABB>& triangle_aabbs,
    const BVHSDFSampler* sampler,
    const BVHSDFSamplerOptions& sampler_options) {
  const AABB parent_bounds =
      octree.nodes[static_cast<std::size_t>(parent_id)].bounds;
  const int parent_level =
      octree.nodes[static_cast<std::size_t>(parent_id)].level;
  std::array<int, 8> child_ids;
  child_ids.fill(-1);
  octree.nodes[static_cast<std::size_t>(parent_id)].is_leaf = false;
  for (int child = 0; child < 8; ++child) {
    AdaptiveOctreeNode node;
    node.id = static_cast<int>(octree.nodes.size());
    node.parent_id = parent_id;
    node.level = parent_level + 1;
    node.bounds = childBounds(parent_bounds, child);
    evaluateNode(
        mesh,
        options,
        triangle_aabbs,
        sampler,
        sampler_options,
        node);
    child_ids[static_cast<std::size_t>(child)] = node.id;
    octree.nodes.push_back(node);
  }
  octree.nodes[static_cast<std::size_t>(parent_id)].children = child_ids;
}

void assignReport(AdaptiveOctreeBuildReport* out, const AdaptiveOctreeBuildReport& value) {
  if (out != nullptr) {
    *out = value;
  }
}

}  // namespace

const char* toString(AdaptiveLeafMode mode) {
  switch (mode) {
    case AdaptiveLeafMode::Mixed:
      return "mixed";
    case AdaptiveLeafMode::Uniform:
      return "uniform";
  }
  return "mixed";
}

bool parseAdaptiveLeafMode(const std::string& value, AdaptiveLeafMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (value == "mixed") {
    *mode = AdaptiveLeafMode::Mixed;
    return true;
  }
  if (value == "uniform" || value == "max-level" || value == "uniform-max") {
    *mode = AdaptiveLeafMode::Uniform;
    return true;
  }
  return false;
}

AdaptiveOctree AdaptiveOctreeBuilder::build(
    const TriangleMesh& mesh,
    const AdaptiveOctreeBuildOptions& options,
    AdaptiveOctreeBuildReport* report_out) {
  AdaptiveOctreeBuildReport report;
  report.min_level = options.min_level;
  report.max_level = options.max_level;
  report.leaf_mode = options.leaf_mode;
  const auto t0 = std::chrono::steady_clock::now();

  AdaptiveOctree octree;
  if (mesh.empty()) {
    report.error_message = "AdaptiveOctreeBuilder requires a non-empty mesh.";
    assignReport(report_out, report);
    return octree;
  }
  if (options.min_level < 0 || options.max_level < options.min_level) {
    report.error_message = "Adaptive octree levels are invalid.";
    assignReport(report_out, report);
    return octree;
  }

  if (options.signed_distance && options.require_watertight_for_signed) {
    const MeshDiagnosticsReport diagnostics = MeshDiagnostics::analyze(mesh);
    if (!diagnostics.watertight) {
      report.error_message =
          "Signed adaptive octree build requires a watertight mesh.";
      assignReport(report_out, report);
      return octree;
    }
  }

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = options.distance_backend;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.bvh_options.degenerate_area_epsilon =
      options.degenerate_area_epsilon;
  BVHSDFSampler sampler;
  const BVHSDFSampler* sampler_ptr = nullptr;
  if (options.distance_backend == SDFSamplingAcceleration::BVH) {
    BuildAccelerationStats acceleration_stats;
    if (sampler.reset(mesh, sampler_options, &acceleration_stats) &&
        sampler.hasBVH()) {
      sampler_ptr = &sampler;
    } else {
      report.warnings.push_back(
          "Adaptive octree BVH distance backend was requested but BVH build "
          "failed; falling back to brute-force refinement sampling.");
    }
  }
  const std::vector<TriangleAABB> triangle_aabbs = makeTriangleAABBs(mesh);

  AdaptiveOctreeNode root;
  root.id = 0;
  root.parent_id = -1;
  root.level = 0;
  root.bounds = paddedRoot(mesh, options.padding);
  evaluateNode(
      mesh,
      options,
      triangle_aabbs,
      sampler_ptr,
      sampler_options,
      root);
  octree.root_id = 0;
  octree.nodes.push_back(root);

  for (std::size_t index = 0; index < octree.nodes.size(); ++index) {
    AdaptiveOctreeNode& node = octree.nodes[index];
    if (shouldRefine(node, options)) {
      const int parent_id = node.id;
      addChildren(
          octree,
          parent_id,
          mesh,
          options,
          triangle_aabbs,
          sampler_ptr,
          sampler_options);
    }
  }

  report.success = true;
  report.node_count = octree.nodeCount();
  report.leaf_count = octree.leafCount();
  report.max_level_used = octree.maxLevelUsed();
  report.near_surface_leaf_count =
      static_cast<std::size_t>(std::count_if(
          octree.nodes.begin(),
          octree.nodes.end(),
          [](const AdaptiveOctreeNode& node) {
            return node.is_leaf && node.near_surface;
          }));
  const auto t1 = std::chrono::steady_clock::now();
  report.build_time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  assignReport(report_out, report);
  return octree;
}

}  // namespace adasdf
