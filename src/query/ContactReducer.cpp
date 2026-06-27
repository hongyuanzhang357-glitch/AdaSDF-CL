#include "adasdf/query/ContactReducer.h"

#include <algorithm>
#include <cmath>

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
    return {1.0, 0.0, 0.0};
  }
  return v / length;
}

Scalar distance(const Vector3& a, const Vector3& b) {
  return norm(a - b);
}

bool sameContactPatch(
    const Contact& kept,
    const RawContactCandidate& candidate,
    const ContactReductionOptions& options) {
  if (options.position_merge_tolerance < 0.0 ||
      options.normal_merge_cosine > 1.0) {
    return false;
  }
  const Vector3 n = normalized(candidate.normal_world);
  return distance(kept.point, candidate.point_world) <=
             options.position_merge_tolerance &&
         dot(kept.normal, n) >= options.normal_merge_cosine;
}

bool rawContactLess(
    const RawContactCandidate& a,
    const RawContactCandidate& b,
    bool prefer_deep_contacts) {
  if (prefer_deep_contacts &&
      std::abs(a.penetration_depth - b.penetration_depth) > 1.0e-12) {
    return a.penetration_depth > b.penetration_depth;
  }
  if (std::abs(a.signed_distance - b.signed_distance) > 1.0e-12) {
    return a.signed_distance < b.signed_distance;
  }
  if (a.source_object_id != b.source_object_id) {
    return a.source_object_id < b.source_object_id;
  }
  if (std::abs(a.point_world.x - b.point_world.x) > 1.0e-12) {
    return a.point_world.x < b.point_world.x;
  }
  if (std::abs(a.point_world.y - b.point_world.y) > 1.0e-12) {
    return a.point_world.y < b.point_world.y;
  }
  return a.point_world.z < b.point_world.z;
}

Contact toContact(
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

}  // namespace

std::vector<Contact> ContactReducer::reduce(
    const std::vector<RawContactCandidate>& raw_contacts,
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const ContactReductionOptions& options) {
  std::vector<Contact> reduced;
  if (options.max_contacts <= 0 || raw_contacts.empty()) {
    return reduced;
  }

  std::vector<RawContactCandidate> sorted = raw_contacts;
  std::sort(sorted.begin(), sorted.end(), [&](const auto& a, const auto& b) {
    return rawContactLess(a, b, options.prefer_deep_contacts);
  });

  for (const RawContactCandidate& raw : sorted) {
    if (reduced.size() >= static_cast<std::size_t>(options.max_contacts)) {
      break;
    }
    if (!raw.point_world.allFinite() || !raw.normal_world.allFinite() ||
        !std::isfinite(raw.signed_distance) ||
        !std::isfinite(raw.penetration_depth)) {
      continue;
    }

    bool merged = false;
    for (const Contact& kept : reduced) {
      if (sameContactPatch(kept, raw, options)) {
        merged = true;
        break;
      }
    }
    if (!merged) {
      reduced.push_back(toContact(raw, obj_a, obj_b));
    }
  }

  return reduced;
}

}  // namespace adasdf
