#include "adasdf/contact/ContactPatch.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

double vectorNorm(const Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 normalizedOrZero(const Vector3& v) {
  const double n = vectorNorm(v);
  if (!(n > 1.0e-20) || !std::isfinite(n)) {
    return {};
  }
  return v / n;
}

bool representativeFirst(const ContactCandidate& a, const ContactCandidate& b) {
  if (a.penetration_depth != b.penetration_depth) {
    return a.penetration_depth > b.penetration_depth;
  }
  return a.sample_id < b.sample_id;
}

}  // namespace

ContactPatch ContactPatchBuilder::fromMembers(
    int patch_id,
    const std::vector<ContactCandidate>& members) {
  ContactPatch patch;
  patch.patch_id = patch_id;
  patch.members = members;
  if (members.empty()) {
    return patch;
  }

  Vector3 centroid_sum;
  Vector3 normal_sum;
  double penetration_sum = 0.0;
  int normal_count = 0;
  patch.max_penetration_depth = members.front().penetration_depth;
  patch.representative_sample_id = members.front().sample_id;

  for (const ContactCandidate& candidate : members) {
    centroid_sum = centroid_sum + candidate.point;
    penetration_sum += candidate.penetration_depth;
    patch.max_penetration_depth =
        std::max(patch.max_penetration_depth, candidate.penetration_depth);
    if (candidate.has_normal && candidate.normal.allFinite()) {
      normal_sum = normal_sum + candidate.normal;
      ++normal_count;
    }
  }

  patch.centroid = centroid_sum / static_cast<double>(members.size());
  patch.average_normal = normal_count > 0 ? normalizedOrZero(normal_sum) : Vector3{};
  patch.mean_penetration_depth =
      penetration_sum / static_cast<double>(members.size());

  const auto iter = std::max_element(
      members.begin(),
      members.end(),
      [](const ContactCandidate& a, const ContactCandidate& b) {
        return representativeFirst(b, a);
      });
  if (iter != members.end()) {
    patch.representative_sample_id = iter->sample_id;
  }
  return patch;
}

}  // namespace adasdf
