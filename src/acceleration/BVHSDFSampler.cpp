#include "adasdf/acceleration/BVHSDFSampler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

#include "adasdf/mesh/MeshSign.h"
#include "adasdf/mesh/TriangleDistance.h"

namespace adasdf {
namespace {

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

double minTriangleDistance(const TriangleMesh& mesh, const Vector3& point) {
  double min_dist = std::numeric_limits<double>::infinity();
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      continue;
    }
    const double dist = pointTriangleDistance(
        point,
        toVector3(mesh.vertices[triangle.v0]),
        toVector3(mesh.vertices[triangle.v1]),
        toVector3(mesh.vertices[triangle.v2]));
    if (std::isfinite(dist)) {
      min_dist = std::min(min_dist, dist);
    }
  }
  return min_dist;
}

void applyBruteSign(
    const TriangleMesh& mesh,
    const Vector3& point,
    double* phi,
    bool* ambiguous) {
  const MeshSignResult sign = MeshSign::classifyPoint(mesh, point);
  if (sign == MeshSignResult::Inside) {
    *phi = -*phi;
  } else if (sign == MeshSignResult::OnSurface) {
    *phi = 0.0;
  } else if (sign == MeshSignResult::Ambiguous && ambiguous != nullptr) {
    *ambiguous = true;
  }
}

}  // namespace

bool BVHSDFSampler::reset(
    const TriangleMesh& mesh,
    const BVHSDFSamplerOptions& options,
    BuildAccelerationStats* stats) {
  mesh_ = &mesh;
  options_ = options;
  bvh_ = TriangleBVH();
  if (stats != nullptr) {
    stats->acceleration = options.acceleration;
    stats->threads_requested = std::max(1, stats->threads_requested);
  }
  if (options.acceleration != SDFSamplingAcceleration::BVH) {
    return true;
  }
  TriangleBVHBuildReport build_report;
  bvh_ = TriangleBVHBuilder::build(mesh, options.bvh_options, &build_report);
  if (stats != nullptr) {
    stats->used_bvh = build_report.success;
    stats->bvh_build_time_ms = build_report.build_time_ms;
    stats->bvh_node_count = build_report.node_count;
    stats->bvh_leaf_count = build_report.leaf_count;
    stats->bvh_triangle_count = build_report.triangle_count;
    for (const std::string& warning : build_report.warnings) {
      stats->warnings.push_back(warning);
    }
    if (!build_report.success && !build_report.error_message.empty()) {
      stats->warnings.push_back(build_report.error_message);
    }
  }
  return build_report.success;
}

BVHSDFSampleResult BVHSDFSampler::sample(const Vector3& point) const {
  if (mesh_ == nullptr) {
    return {};
  }
  if (options_.acceleration == SDFSamplingAcceleration::BVH &&
      bvh_.isValid()) {
    return sampleWithBVH(*mesh_, bvh_, point, options_);
  }
  return sampleBruteForce(*mesh_, point, options_.signed_distance);
}

BVHSDFSampleResult BVHSDFSampler::sampleBruteForce(
    const TriangleMesh& mesh,
    const Vector3& point,
    bool signed_distance) {
  BVHSDFSampleResult result;
  double phi = minTriangleDistance(mesh, point);
  if (!std::isfinite(phi)) {
    phi = 0.0;
  }
  if (signed_distance) {
    applyBruteSign(mesh, point, &phi, &result.ambiguous_sign);
  }
  result.success = true;
  result.phi = phi;
  return result;
}

BVHSDFSampleResult BVHSDFSampler::sampleWithBVH(
    const TriangleMesh& mesh,
    const TriangleBVH& bvh,
    const Vector3& point,
    const BVHSDFSamplerOptions& options) {
  BVHSDFSampleResult result;
  result.used_bvh = bvh.isValid();
  if (!bvh.isValid()) {
    return sampleBruteForce(mesh, point, options.signed_distance);
  }
  result.nearest = BVHNearestTriangleQuery::query(bvh, point);
  if (!result.nearest.success) {
    return sampleBruteForce(mesh, point, options.signed_distance);
  }
  double phi = result.nearest.distance;
  if (!std::isfinite(phi)) {
    phi = 0.0;
  }
  if (options.signed_distance) {
    if (std::abs(phi) <= options.surface_epsilon) {
      phi = 0.0;
    } else {
      const BVHRay ray{point, {1.0, 0.0, 0.0}};
      result.ray = BVHRayIntersectionQuery::countIntersections(bvh, ray);
      if (!result.ray.success || result.ray.ambiguous) {
        result.fallback_sign = true;
        if (options.fallback_to_bruteforce_sign) {
          applyBruteSign(mesh, point, &phi, &result.ambiguous_sign);
        } else {
          result.ambiguous_sign = true;
        }
      } else if ((result.ray.hit_count % 2) == 1) {
        phi = -phi;
      }
    }
  }
  result.success = true;
  result.phi = phi;
  return result;
}

}  // namespace adasdf
