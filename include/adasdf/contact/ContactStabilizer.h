#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/contact/ContactBudget.h"
#include "adasdf/contact/ContactClusterer.h"

namespace adasdf {

struct ContactStabilizationOptions {
  ContactBudget budget;
  ContactPatchOptions patch_options;

  bool cluster_candidates = true;
  bool enforce_normal_consistency = true;
  bool remove_duplicate_samples = true;

  double duplicate_position_epsilon = 1e-9;
  double min_penetration_depth = 0.0;

  // If true, choose representative candidate from each patch before applying budget.
  bool use_patch_representatives = true;

  // If true, final output is sorted by penetration depth descending, tie by sample_id.
  bool deterministic_sort = true;
};

struct ContactStabilizationStats {
  std::size_t input_candidate_count = 0;
  std::size_t after_duplicate_removal_count = 0;
  std::size_t after_threshold_count = 0;
  std::size_t patch_count = 0;
  std::size_t output_contact_count = 0;

  ContactBudgetStats budget_stats;

  std::vector<std::string> warnings;
};

struct ContactStabilizationResult {
  bool success = false;
  std::string error_message;

  std::vector<ContactCandidate> stabilized_candidates;
  std::vector<ContactPatch> patches;

  ContactStabilizationStats stats;
};

class ContactStabilizer {
 public:
  static ContactStabilizationResult stabilize(
      const std::vector<ContactCandidate>& candidates,
      const ContactStabilizationOptions& options = ContactStabilizationOptions{});
};

}  // namespace adasdf
