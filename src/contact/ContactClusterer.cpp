#include "adasdf/contact/ContactClusterer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

double distance(const Vector3& a, const Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool hasPatchNormal(const ContactPatch& patch) {
  return patch.average_normal.allFinite() &&
         (std::abs(patch.average_normal.x) > 0.0 ||
          std::abs(patch.average_normal.y) > 0.0 ||
          std::abs(patch.average_normal.z) > 0.0);
}

bool normalsCompatible(
    const ContactCandidate& candidate,
    const ContactPatch& patch,
    const ContactPatchOptions& options) {
  if (!options.require_normal_consistency) {
    return true;
  }
  if (!candidate.has_normal || !candidate.normal.allFinite() ||
      !hasPatchNormal(patch)) {
    return false;
  }
  return dot(candidate.normal, patch.average_normal) >=
         options.normal_cosine_threshold;
}

bool deeperFirst(const ContactCandidate& a, const ContactCandidate& b) {
  if (a.penetration_depth != b.penetration_depth) {
    return a.penetration_depth > b.penetration_depth;
  }
  return a.sample_id < b.sample_id;
}

void refreshPatch(ContactPatch& patch) {
  patch = ContactPatchBuilder::fromMembers(patch.patch_id, patch.members);
}

}  // namespace

ContactClusterResult ContactClusterer::cluster(
    const std::vector<ContactCandidate>& candidates,
    const ContactPatchOptions& options) {
  ContactClusterResult result;
  result.input_candidate_count = candidates.size();

  if (options.max_patches <= 0) {
    result.warnings.push_back("max_patches <= 0; no patches emitted");
    return result;
  }

  std::vector<ContactCandidate> sorted = candidates;
  if (options.deterministic) {
    std::sort(sorted.begin(), sorted.end(), deeperFirst);
  }

  bool overflow_warned = false;
  bool missing_normal_warned = false;
  for (const ContactCandidate& candidate : sorted) {
    int best_patch = -1;
    double best_distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < result.patches.size(); ++i) {
      const ContactPatch& patch = result.patches[i];
      const double d = distance(candidate.point, patch.centroid);
      if (d <= options.spatial_radius &&
          normalsCompatible(candidate, patch, options) &&
          d < best_distance) {
        best_patch = static_cast<int>(i);
        best_distance = d;
      }
    }

    if (best_patch >= 0) {
      result.patches[static_cast<std::size_t>(best_patch)].members.push_back(candidate);
      refreshPatch(result.patches[static_cast<std::size_t>(best_patch)]);
      continue;
    }

    if (options.require_normal_consistency && !candidate.has_normal &&
        !missing_normal_warned) {
      result.warnings.push_back(
          "candidate without normal could not be normal-clustered");
      missing_normal_warned = true;
    }

    if (static_cast<int>(result.patches.size()) < options.max_patches) {
      const int patch_id = static_cast<int>(result.patches.size());
      result.patches.push_back(
          ContactPatchBuilder::fromMembers(patch_id, {candidate}));
      continue;
    }

    int nearest_patch = -1;
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < result.patches.size(); ++i) {
      const double d = distance(candidate.point, result.patches[i].centroid);
      if (d < nearest_distance) {
        nearest_patch = static_cast<int>(i);
        nearest_distance = d;
      }
    }
    if (nearest_patch >= 0) {
      result.patches[static_cast<std::size_t>(nearest_patch)].members.push_back(candidate);
      refreshPatch(result.patches[static_cast<std::size_t>(nearest_patch)]);
      if (!overflow_warned) {
        result.warnings.push_back(
            "max_patches reached; overflow candidates assigned to nearest patch");
        overflow_warned = true;
      }
    }
  }

  result.patches.erase(
      std::remove_if(
          result.patches.begin(),
          result.patches.end(),
          [](const ContactPatch& patch) { return patch.members.empty(); }),
      result.patches.end());
  for (std::size_t i = 0; i < result.patches.size(); ++i) {
    result.patches[i].patch_id = static_cast<int>(i);
    refreshPatch(result.patches[i]);
  }
  result.patch_count = result.patches.size();
  return result;
}

}  // namespace adasdf
