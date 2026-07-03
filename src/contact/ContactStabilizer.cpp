#include "adasdf/contact/ContactStabilizer.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

double distance(const Vector3& a, const Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool deeperFirst(const ContactCandidate& a, const ContactCandidate& b) {
  if (a.penetration_depth != b.penetration_depth) {
    return a.penetration_depth > b.penetration_depth;
  }
  return a.sample_id < b.sample_id;
}

const ContactCandidate* findRepresentative(const ContactPatch& patch) {
  const ContactCandidate* best = nullptr;
  for (const ContactCandidate& member : patch.members) {
    if (!best || deeperFirst(member, *best)) {
      best = &member;
    }
  }
  return best;
}

}  // namespace

ContactStabilizationResult ContactStabilizer::stabilize(
    const std::vector<ContactCandidate>& candidates,
    const ContactStabilizationOptions& options) {
  ContactStabilizationResult result;
  result.stats.input_candidate_count = candidates.size();

  std::vector<ContactCandidate> working = candidates;
  std::sort(working.begin(), working.end(), deeperFirst);

  if (options.remove_duplicate_samples) {
    std::vector<ContactCandidate> unique;
    for (const ContactCandidate& candidate : working) {
      bool duplicate = false;
      for (const ContactCandidate& kept : unique) {
        if (candidate.sample_id >= 0 && candidate.sample_id == kept.sample_id) {
          duplicate = true;
          break;
        }
        if (options.duplicate_position_epsilon > 0.0 &&
            distance(candidate.point, kept.point) <
                options.duplicate_position_epsilon) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) {
        unique.push_back(candidate);
      }
    }
    working = unique;
  }
  result.stats.after_duplicate_removal_count = working.size();

  std::vector<ContactCandidate> thresholded;
  for (const ContactCandidate& candidate : working) {
    if (candidate.penetration_depth >= options.min_penetration_depth) {
      thresholded.push_back(candidate);
    }
  }
  result.stats.after_threshold_count = thresholded.size();

  std::vector<ContactCandidate> budget_input = thresholded;
  if (options.cluster_candidates) {
    ContactPatchOptions patch_options = options.patch_options;
    patch_options.require_normal_consistency =
        options.enforce_normal_consistency &&
        patch_options.require_normal_consistency;
    ContactClusterResult clustered =
        ContactClusterer::cluster(thresholded, patch_options);
    result.patches = clustered.patches;
    result.stats.patch_count = clustered.patch_count;
    result.stats.warnings.insert(
        result.stats.warnings.end(),
        clustered.warnings.begin(),
        clustered.warnings.end());

    if (options.use_patch_representatives) {
      budget_input.clear();
      for (const ContactPatch& patch : result.patches) {
        const ContactCandidate* representative = findRepresentative(patch);
        if (representative) {
          budget_input.push_back(*representative);
        }
      }
    }
  }

  ContactBudget budget = options.budget;
  if (options.cluster_candidates && options.use_patch_representatives) {
    budget.enforce_patch_budget = false;
  }
  result.stabilized_candidates =
      ContactBudgetApplier::apply(budget_input, budget, &result.stats.budget_stats);
  result.stats.output_contact_count = result.stabilized_candidates.size();
  result.stats.warnings.insert(
      result.stats.warnings.end(),
      result.stats.budget_stats.warnings.begin(),
      result.stats.budget_stats.warnings.end());

  if (options.deterministic_sort) {
    std::sort(
        result.stabilized_candidates.begin(),
        result.stabilized_candidates.end(),
        deeperFirst);
  }

  result.success = true;
  return result;
}

}  // namespace adasdf
