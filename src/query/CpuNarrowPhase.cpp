#include "adasdf/query/CpuNarrowPhase.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

#include "adasdf/query/ContactGenerator.h"
#include "adasdf/query/ContactReducer.h"

namespace adasdf {
namespace {

struct ClosestAABBPoints {
  Vector3 point_a;
  Vector3 point_b;
  Vector3 normal;
  Scalar distance = 0.0;
};

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

bool finiteAABB(const AABB& bounds) {
  return bounds.valid && bounds.min.allFinite() && bounds.max.allFinite() &&
         bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
         bounds.min.z <= bounds.max.z;
}

bool aabbOverlap(const AABB& a, const AABB& b, Scalar pad) {
  if (!finiteAABB(a) || !finiteAABB(b)) {
    return false;
  }
  return a.min.x <= b.max.x + pad && a.max.x + pad >= b.min.x &&
         a.min.y <= b.max.y + pad && a.max.y + pad >= b.min.y &&
         a.min.z <= b.max.z + pad && a.max.z + pad >= b.min.z;
}

Scalar axisGap(Scalar min_a, Scalar max_a, Scalar min_b, Scalar max_b) {
  if (max_a < min_b) {
    return min_b - max_a;
  }
  if (max_b < min_a) {
    return min_a - max_b;
  }
  return 0.0;
}

Scalar aabbDistance(const AABB& a, const AABB& b) {
  if (!finiteAABB(a) || !finiteAABB(b)) {
    return std::numeric_limits<Scalar>::infinity();
  }
  const Scalar dx = axisGap(a.min.x, a.max.x, b.min.x, b.max.x);
  const Scalar dy = axisGap(a.min.y, a.max.y, b.min.y, b.max.y);
  const Scalar dz = axisGap(a.min.z, a.max.z, b.min.z, b.max.z);
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Scalar closestAxisPoint(Scalar min_a, Scalar max_a, Scalar min_b, Scalar max_b) {
  const Scalar overlap_min = std::max(min_a, min_b);
  const Scalar overlap_max = std::min(max_a, max_b);
  if (overlap_min <= overlap_max) {
    return 0.5 * (overlap_min + overlap_max);
  }
  return max_a < min_b ? max_a : min_a;
}

ClosestAABBPoints closestAABBPoints(const AABB& a, const AABB& b) {
  ClosestAABBPoints result;
  result.distance = aabbDistance(a, b);

  result.point_a = {
      closestAxisPoint(a.min.x, a.max.x, b.min.x, b.max.x),
      closestAxisPoint(a.min.y, a.max.y, b.min.y, b.max.y),
      closestAxisPoint(a.min.z, a.max.z, b.min.z, b.max.z)};
  result.point_b = result.point_a;

  if (a.max.x < b.min.x) {
    result.point_a.x = a.max.x;
    result.point_b.x = b.min.x;
  } else if (b.max.x < a.min.x) {
    result.point_a.x = a.min.x;
    result.point_b.x = b.max.x;
  }

  if (a.max.y < b.min.y) {
    result.point_a.y = a.max.y;
    result.point_b.y = b.min.y;
  } else if (b.max.y < a.min.y) {
    result.point_a.y = a.min.y;
    result.point_b.y = b.max.y;
  }

  if (a.max.z < b.min.z) {
    result.point_a.z = a.max.z;
    result.point_b.z = b.min.z;
  } else if (b.max.z < a.min.z) {
    result.point_a.z = a.min.z;
    result.point_b.z = b.max.z;
  }

  result.normal = normalized(result.point_b - result.point_a);
  return result;
}

bool modelsReady(const CollisionObject& obj_a, const CollisionObject& obj_b) {
  return obj_a.getModel() && obj_b.getModel() &&
         obj_a.getModel()->queryBackendAvailable() &&
         obj_b.getModel()->queryBackendAvailable();
}

ContactReductionOptions reductionOptionsForRequest(const CollisionRequest& request) {
  ContactReductionOptions options;
  options.max_contacts =
      request.enable_contact && request.max_contacts > 0 ? request.max_contacts : 0;
  options.position_merge_tolerance = request.enable_contact_reduction
                                         ? request.contact_merge_tolerance
                                         : -1.0;
  options.normal_merge_cosine =
      request.enable_contact_reduction ? request.normal_merge_cosine : 2.0;
  options.prefer_deep_contacts = true;
  return options;
}

void applyStats(const NarrowPhaseStats& stats, CollisionResult& result) {
  result.setNumCandidatePoints(stats.num_candidate_points);
  result.setNumSDFQueries(stats.num_sdf_queries);
  result.setNumRawContacts(stats.num_raw_contacts);
  result.setNumReducedContacts(stats.num_reduced_contacts);
  result.setMethodInfo(stats.method);
}

void applyStats(const NarrowPhaseStats& stats, DistanceResult& result) {
  result.setNumCandidatePoints(stats.num_candidate_points);
  result.setNumSDFQueries(stats.num_sdf_queries);
  result.setNumRawContacts(stats.num_raw_contacts);
  result.setNumReducedContacts(stats.num_reduced_contacts);
  result.setMethodInfo(stats.method);
}

bool betterDistanceSample(
    const RawContactCandidate& candidate,
    const RawContactCandidate& current,
    bool has_current) {
  if (!std::isfinite(candidate.signed_distance)) {
    return false;
  }
  if (!has_current) {
    return true;
  }
  return candidate.signed_distance < current.signed_distance;
}

std::vector<RawContactCandidate> filterContacts(
    const std::vector<RawContactCandidate>& samples,
    Scalar contact_tolerance) {
  std::vector<RawContactCandidate> contacts;
  for (const RawContactCandidate& sample : samples) {
    if (sample.signed_distance <= contact_tolerance) {
      contacts.push_back(sample);
    }
  }
  return contacts;
}

Contact closestContactFromRaw(
    const RawContactCandidate& raw,
    const CollisionObject& obj_a,
    const CollisionObject& obj_b) {
  Contact contact;
  contact.point = raw.point_world;
  contact.normal = normalized(raw.normal_world);
  contact.penetration_depth = std::max<Scalar>(0.0, raw.penetration_depth);
  contact.signed_distance = raw.signed_distance;
  contact.object_id_a = obj_a.id();
  contact.object_id_b = obj_b.id();
  contact.gradient = contact.normal;
  return contact;
}

void nearestPointsFromRaw(
    const RawContactCandidate& raw,
    const CollisionObject& obj_a,
    Vector3& nearest_a,
    Vector3& nearest_b) {
  const Vector3 n = normalized(raw.normal_world);
  if (raw.source_object_id == obj_a.id()) {
    nearest_a = raw.point_world;
    nearest_b = raw.point_world - raw.signed_distance * n;
  } else {
    nearest_b = raw.point_world;
    nearest_a = raw.point_world + raw.signed_distance * n;
  }
}

}  // namespace

bool CpuNarrowPhase::collide(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const CollisionRequest& request,
    CollisionResult& result) {
  result.clear();
  result.setBackendInfo("CPU narrow-phase: symmetric SDF-sampling research preview");

  const auto start = std::chrono::steady_clock::now();
  const AABB aabb_a = obj_a.getAABB();
  const AABB aabb_b = obj_b.getAABB();
  if (!modelsReady(obj_a, obj_b) || !finiteAABB(aabb_a) || !finiteAABB(aabb_b)) {
    result.setMinimumDistance(std::numeric_limits<Scalar>::infinity());
    return false;
  }

  if (!aabbOverlap(aabb_a, aabb_b, request.contact_tolerance)) {
    result.setColliding(false);
    result.setMinimumDistance(aabbDistance(aabb_a, aabb_b));
    result.setMethodInfo("AABB broadphase lower-bound distance");
    return false;
  }

  // Research-preview CPU narrow-phase: deterministic candidate sampling,
  // symmetric SDF queries, and heuristic contact reduction. It is intended for
  // v0.5 contact experiments, not as a certified industrial narrow-phase.
  CollisionRequest sample_request = request;
  sample_request.contact_tolerance = std::numeric_limits<Scalar>::infinity();

  NarrowPhaseStats stats;
  std::vector<RawContactCandidate> all_samples =
      ContactGenerator::generateSymmetricContacts(obj_a, obj_b, sample_request, stats);
  std::vector<RawContactCandidate> raw_contacts =
      filterContacts(all_samples, request.contact_tolerance);
  stats.num_raw_contacts = raw_contacts.size();
  stats.method += " + deterministic contact reduction";

  RawContactCandidate best;
  bool has_best = false;
  for (const RawContactCandidate& sample : all_samples) {
    if (betterDistanceSample(sample, best, has_best)) {
      best = sample;
      has_best = true;
    }
  }

  const ContactReductionOptions reduction_options = reductionOptionsForRequest(request);
  const std::vector<Contact> contacts =
      ContactReducer::reduce(raw_contacts, obj_a, obj_b, reduction_options);
  stats.num_reduced_contacts = contacts.size();

  if (request.enable_contact) {
    for (const Contact& contact : contacts) {
      result.addContact(contact);
    }
  }

  const bool hit = !raw_contacts.empty();
  result.setColliding(hit);
  result.setMinimumDistance(has_best ? best.signed_distance : 0.0);
  applyStats(stats, result);

  QueryTiming timing;
  timing.total_seconds = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start)
                             .count();
  timing.narrowphase_seconds = timing.total_seconds;
  result.setTiming(timing);
  return hit;
}

Scalar CpuNarrowPhase::distance(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const DistanceRequest& request,
    DistanceResult& result) {
  result.clear();
  result.setBackendInfo("CPU narrow-phase: approximate SDF-sampling distance");

  const AABB aabb_a = obj_a.getAABB();
  const AABB aabb_b = obj_b.getAABB();
  if (!modelsReady(obj_a, obj_b) || !finiteAABB(aabb_a) || !finiteAABB(aabb_b)) {
    result.setMinimumDistance(std::numeric_limits<Scalar>::infinity());
    return result.minimumDistance();
  }

  if (!aabbOverlap(aabb_a, aabb_b, request.distance_tolerance)) {
    const ClosestAABBPoints closest = closestAABBPoints(aabb_a, aabb_b);
    result.setMinimumDistance(closest.distance);
    result.setNearestPoints(closest.point_a, closest.point_b);
    result.setNormal(closest.normal);
    result.setNumSDFQueries(0);
    result.setMethodInfo("AABB broadphase lower-bound distance");
    return closest.distance;
  }

  CollisionRequest proxy;
  proxy.backend = request.backend;
  proxy.query_mode = request.query_mode;
  proxy.enable_contact = true;
  proxy.enable_gradient = request.enable_gradient;
  proxy.contact_tolerance = std::numeric_limits<Scalar>::infinity();
  proxy.symmetric_query = true;
  proxy.near_surface_band = std::max<Scalar>(request.distance_tolerance, 1.0e-4);

  NarrowPhaseStats stats;
  std::vector<RawContactCandidate> samples =
      ContactGenerator::generateSymmetricContacts(obj_a, obj_b, proxy, stats);
  stats.method += " + best signed-distance sample";

  RawContactCandidate best;
  bool has_best = false;
  for (const RawContactCandidate& sample : samples) {
    if (betterDistanceSample(sample, best, has_best)) {
      best = sample;
      has_best = true;
    }
  }
  stats.num_raw_contacts = samples.size();
  stats.num_reduced_contacts = 0;
  applyStats(stats, result);

  if (!has_best) {
    const ClosestAABBPoints closest = closestAABBPoints(aabb_a, aabb_b);
    result.setMinimumDistance(closest.distance);
    result.setNearestPoints(closest.point_a, closest.point_b);
    result.setNormal(closest.normal);
    return closest.distance;
  }

  Vector3 nearest_a;
  Vector3 nearest_b;
  nearestPointsFromRaw(best, obj_a, nearest_a, nearest_b);

  result.setMinimumDistance(best.signed_distance);
  result.setNearestPoints(nearest_a, nearest_b);
  result.setNormal(normalized(best.normal_world));
  result.setClosestContact(closestContactFromRaw(best, obj_a, obj_b));
  return best.signed_distance;
}

}  // namespace adasdf
