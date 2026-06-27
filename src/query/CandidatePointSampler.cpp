#include "adasdf/query/CandidatePointSampler.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

Scalar dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Scalar norm(const Vector3& v) {
  return std::sqrt(dot(v, v));
}

Vector3 center(const AABB& bounds) {
  return 0.5 * (bounds.min + bounds.max);
}

bool finiteAABB(const AABB& bounds) {
  return bounds.valid && bounds.min.allFinite() && bounds.max.allFinite() &&
         bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
         bounds.min.z <= bounds.max.z;
}

Scalar maxExtent(const AABB& bounds) {
  const Vector3 diag = bounds.max - bounds.min;
  return std::max({std::abs(diag.x), std::abs(diag.y), std::abs(diag.z)});
}

bool closePoint(const Vector3& a, const Vector3& b, Scalar tolerance) {
  return norm(a - b) <= tolerance;
}

void addPoint(
    std::vector<Vector3>& points,
    const Vector3& point,
    int max_points,
    Scalar duplicate_tolerance) {
  if (max_points <= 0 || points.size() >= static_cast<std::size_t>(max_points) ||
      !point.allFinite()) {
    return;
  }
  for (const Vector3& existing : points) {
    if (closePoint(existing, point, duplicate_tolerance)) {
      return;
    }
  }
  points.push_back(point);
}

bool nearSourceSurface(
    const CollisionObject& object,
    const Vector3& point_world,
    Scalar band) {
  const auto& model = object.getModel();
  if (!model || !model->queryBackendAvailable()) {
    return true;
  }
  const Vector3 local = object.getTransform().inverseApplyPoint(point_world);
  try {
    const Scalar phi = model->sampleDistance(local);
    return std::isfinite(phi) && std::abs(phi) <= band;
  } catch (const std::exception&) {
    return true;
  }
}

bool shouldUseGrid(CandidateSamplingMode mode, const CandidateSamplingOptions& options) {
  return options.include_aabb_grid &&
         (mode == CandidateSamplingMode::Auto ||
          mode == CandidateSamplingMode::AABBGrid ||
          mode == CandidateSamplingMode::SurfaceApprox ||
          mode == CandidateSamplingMode::NearSurfaceApprox);
}

}  // namespace

std::vector<Vector3> CandidatePointSampler::sampleWorldPoints(
    const CollisionObject& object,
    const CandidateSamplingOptions& options) {
  std::vector<Vector3> points;
  const auto& model = object.getModel();
  if (!model || options.max_points <= 0) {
    return points;
  }

  const AABB local = model->boundingBox();
  if (!finiteAABB(local)) {
    return points;
  }

  const CandidateSamplingMode mode = options.mode;
  const Transform& transform = object.getTransform();
  const Scalar extent = std::max<Scalar>(maxExtent(local), 1.0e-9);
  const Scalar duplicate_tolerance = extent * 1.0e-10;
  const int resolution = std::max(2, options.grid_resolution);
  const Scalar effective_band =
      std::max(options.surface_band, extent / static_cast<Scalar>(2 * resolution));

  auto addLocal = [&](const Vector3& p) {
    addPoint(points, transform.applyPoint(p), options.max_points, duplicate_tolerance);
  };

  if (options.include_aabb_corners &&
      (mode == CandidateSamplingMode::Auto ||
       mode == CandidateSamplingMode::AABBCorners ||
       mode == CandidateSamplingMode::AABBGrid ||
       mode == CandidateSamplingMode::SurfaceApprox ||
       mode == CandidateSamplingMode::NearSurfaceApprox)) {
    addLocal({local.min.x, local.min.y, local.min.z});
    addLocal({local.max.x, local.min.y, local.min.z});
    addLocal({local.min.x, local.max.y, local.min.z});
    addLocal({local.max.x, local.max.y, local.min.z});
    addLocal({local.min.x, local.min.y, local.max.z});
    addLocal({local.max.x, local.min.y, local.max.z});
    addLocal({local.min.x, local.max.y, local.max.z});
    addLocal({local.max.x, local.max.y, local.max.z});
  }

  const Vector3 c = center(local);
  if (mode == CandidateSamplingMode::Auto ||
      mode == CandidateSamplingMode::SurfaceApprox ||
      mode == CandidateSamplingMode::NearSurfaceApprox) {
    addLocal(c);
  }

  if (options.include_aabb_face_centers &&
      mode != CandidateSamplingMode::AABBCorners) {
    addLocal({local.min.x, c.y, c.z});
    addLocal({local.max.x, c.y, c.z});
    addLocal({c.x, local.min.y, c.z});
    addLocal({c.x, local.max.y, c.z});
    addLocal({c.x, c.y, local.min.z});
    addLocal({c.x, c.y, local.max.z});
  }

  if (!shouldUseGrid(mode, options)) {
    return points;
  }

  const Vector3 diag = local.max - local.min;
  for (int ix = 0; ix < resolution &&
                   points.size() < static_cast<std::size_t>(options.max_points);
       ++ix) {
    const Scalar tx = resolution == 1 ? 0.0
                                      : static_cast<Scalar>(ix) /
                                            static_cast<Scalar>(resolution - 1);
    for (int iy = 0; iy < resolution &&
                     points.size() < static_cast<std::size_t>(options.max_points);
         ++iy) {
      const Scalar ty = resolution == 1 ? 0.0
                                        : static_cast<Scalar>(iy) /
                                              static_cast<Scalar>(resolution - 1);
      for (int iz = 0; iz < resolution &&
                       points.size() < static_cast<std::size_t>(options.max_points);
           ++iz) {
        const Scalar tz = resolution == 1 ? 0.0
                                          : static_cast<Scalar>(iz) /
                                                static_cast<Scalar>(resolution - 1);
        const Vector3 local_point{
            local.min.x + tx * diag.x,
            local.min.y + ty * diag.y,
            local.min.z + tz * diag.z};
        const Vector3 world_point = transform.applyPoint(local_point);
        if (nearSourceSurface(object, world_point, effective_band)) {
          addPoint(points, world_point, options.max_points, duplicate_tolerance);
        }
      }
    }
  }

  return points;
}

}  // namespace adasdf
