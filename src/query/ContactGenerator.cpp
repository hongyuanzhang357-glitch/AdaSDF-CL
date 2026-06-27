#include "adasdf/query/ContactGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "adasdf/query/CandidatePointSampler.h"

namespace adasdf {
namespace {

Scalar dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Scalar norm(const Vector3& v) {
  return std::sqrt(dot(v, v));
}

Vector3 normalized(const Vector3& v) {
  const Scalar length = norm(v);
  if (!(length > 1.0e-12) || !v.allFinite()) {
    return Vector3::Zero();
  }
  return v / length;
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

bool insideAABB(const AABB& bounds, const Vector3& point, Scalar pad) {
  if (!finiteAABB(bounds)) {
    return false;
  }
  return point.x >= bounds.min.x - pad && point.x <= bounds.max.x + pad &&
         point.y >= bounds.min.y - pad && point.y <= bounds.max.y + pad &&
         point.z >= bounds.min.z - pad && point.z <= bounds.max.z + pad;
}

int modeGridResolution(QueryMode mode) {
  switch (mode) {
    case QueryMode::Fast:
      return 4;
    case QueryMode::Accurate:
      return 16;
    case QueryMode::Balanced:
    default:
      return 8;
  }
}

int modeMaxCandidatePoints(QueryMode mode) {
  switch (mode) {
    case QueryMode::Fast:
      return 512;
    case QueryMode::Accurate:
      return 20000;
    case QueryMode::Balanced:
    default:
      return 4096;
  }
}

CandidateSamplingOptions samplingOptionsForRequest(
    const CollisionRequest& request,
    bool symmetric) {
  CandidateSamplingOptions options;
  const int mode_grid = modeGridResolution(request.query_mode);
  const int mode_max = modeMaxCandidatePoints(request.query_mode);
  options.grid_resolution =
      request.candidate_grid_resolution == 8 ? mode_grid
                                             : request.candidate_grid_resolution;
  options.max_points =
      request.max_candidate_points == 4096 ? mode_max
                                           : request.max_candidate_points;
  if (symmetric) {
    options.max_points = std::max(1, options.max_points / 2);
  }
  options.surface_band = request.near_surface_band;
  options.mode = CandidateSamplingMode::Auto;
  return options;
}

Vector3 fallbackNormalBToA(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const Vector3& point_world,
    const CollisionObject& target) {
  const auto& target_model = target.getModel();
  if (target_model) {
    const Vector3 target_center =
        target.getTransform().applyPoint(center(target_model->boundingBox()));
    const Vector3 from_target = normalized(point_world - target_center);
    if (norm(from_target) > 1.0e-12) {
      if (target.id() == obj_b.id()) {
        return from_target;
      }
      return -1.0 * from_target;
    }
  }

  const auto& model_a = obj_a.getModel();
  const auto& model_b = obj_b.getModel();
  if (model_a && model_b) {
    const Vector3 center_a =
        obj_a.getTransform().applyPoint(center(model_a->boundingBox()));
    const Vector3 center_b =
        obj_b.getTransform().applyPoint(center(model_b->boundingBox()));
    const Vector3 n = normalized(center_a - center_b);
    if (norm(n) > 1.0e-12) {
      return n;
    }
  }
  return {1.0, 0.0, 0.0};
}

Vector3 centerDirectionBToA(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b) {
  const auto& model_a = obj_a.getModel();
  const auto& model_b = obj_b.getModel();
  if (!model_a || !model_b) {
    return Vector3::Zero();
  }
  const Vector3 center_a =
      obj_a.getTransform().applyPoint(center(model_a->boundingBox()));
  const Vector3 center_b =
      obj_b.getTransform().applyPoint(center(model_b->boundingBox()));
  return normalized(center_a - center_b);
}

bool evaluatePoint(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const CollisionObject& source,
    const CollisionObject& target,
    const Vector3& point_world,
    const CollisionRequest& request,
    RawContactCandidate& out,
    NarrowPhaseStats& stats) {
  const auto& target_model = target.getModel();
  if (!target_model || !target_model->queryBackendAvailable()) {
    return false;
  }

  const AABB target_bounds = target_model->boundingBox();
  const Scalar pad =
      std::max({std::abs(request.contact_tolerance),
                std::abs(request.near_surface_band),
                maxExtent(target_bounds) * 1.0e-6,
                1.0e-9});
  const Vector3 target_local = target.getTransform().inverseApplyPoint(point_world);
  if (!insideAABB(target_bounds, target_local, pad)) {
    return false;
  }

  Scalar phi = std::numeric_limits<Scalar>::infinity();
  Vector3 gradient_world;
  try {
    phi = target_model->sampleDistance(target_local);
    ++stats.num_sdf_queries;
    const Vector3 gradient_local = target_model->sampleGradient(target_local);
    stats.num_sdf_queries += 6;
    gradient_world = target.getTransform().applyVector(gradient_local);
  } catch (const std::exception&) {
    return false;
  }

  if (!std::isfinite(phi) || !point_world.allFinite()) {
    return false;
  }

  Vector3 normal_target_out = normalized(gradient_world);
  if (norm(normal_target_out) <= 1.0e-12) {
    normal_target_out = fallbackNormalBToA(obj_a, obj_b, point_world, target);
    if (target.id() == obj_a.id()) {
      normal_target_out = -1.0 * normal_target_out;
    }
  }

  // Contact normals are reported in world coordinates from object B toward A.
  // A sample from A queried against B uses B's outward SDF normal directly.
  // A sample from B queried against A flips A's outward normal to keep the
  // convention stable within one query.
  const bool source_is_a = source.id() == obj_a.id();
  Vector3 normal_b_to_a = source_is_a ? normal_target_out : -1.0 * normal_target_out;
  normal_b_to_a = normalized(normal_b_to_a);
  if (norm(normal_b_to_a) <= 1.0e-12) {
    normal_b_to_a = fallbackNormalBToA(obj_a, obj_b, point_world, target);
  }
  const Vector3 center_direction = centerDirectionBToA(obj_a, obj_b);
  if (norm(center_direction) > 1.0e-12 && dot(normal_b_to_a, center_direction) < 0.0) {
    normal_b_to_a = -1.0 * normal_b_to_a;
  }

  out.point_world = point_world;
  out.normal_world = normal_b_to_a;
  out.signed_distance = phi;
  out.penetration_depth = std::max<Scalar>(0.0, -phi);
  out.source_object_id = source.id();
  out.target_object_id = target.id();
  return true;
}

void evaluateDirection(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const CollisionObject& source,
    const CollisionObject& target,
    const CollisionRequest& request,
    const CandidateSamplingOptions& sampling_options,
    std::vector<RawContactCandidate>& contacts,
    NarrowPhaseStats& stats) {
  const std::vector<Vector3> points =
      CandidatePointSampler::sampleWorldPoints(source, sampling_options);
  stats.num_candidate_points += points.size();

  for (const Vector3& point : points) {
    RawContactCandidate candidate;
    if (!evaluatePoint(obj_a, obj_b, source, target, point, request, candidate, stats)) {
      continue;
    }
    if (candidate.signed_distance <= request.contact_tolerance) {
      contacts.push_back(candidate);
    }
  }
}

}  // namespace

std::vector<RawContactCandidate> ContactGenerator::generateSymmetricContacts(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const CollisionRequest& request,
    NarrowPhaseStats& stats) {
  std::vector<RawContactCandidate> contacts;
  stats.used_symmetric_query = request.symmetric_query;
  stats.method = request.symmetric_query
                     ? "symmetric candidate-point SDF query"
                     : "one-way candidate-point SDF query";

  const CandidateSamplingOptions sampling_options =
      samplingOptionsForRequest(request, request.symmetric_query);

  evaluateDirection(
      obj_a, obj_b, obj_a, obj_b, request, sampling_options, contacts, stats);
  if (request.symmetric_query) {
    evaluateDirection(
        obj_a, obj_b, obj_b, obj_a, request, sampling_options, contacts, stats);
  }

  stats.num_raw_contacts = contacts.size();
  return contacts;
}

}  // namespace adasdf
