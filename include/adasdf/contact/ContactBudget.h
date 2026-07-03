#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/sparse/ContactCandidateReducer.h"

namespace adasdf {

struct ContactBudget {
  int max_contacts_total = 8;
  int max_contacts_per_object_pair = 8;
  int max_contacts_per_link = 4;
  int max_contacts_per_patch = 2;

  bool enforce_total_budget = true;
  bool enforce_link_budget = false;
  bool enforce_patch_budget = true;
};

struct ContactBudgetStats {
  std::size_t input_candidate_count = 0;
  std::size_t after_total_budget_count = 0;
  std::size_t after_link_budget_count = 0;
  std::size_t after_patch_budget_count = 0;

  std::vector<std::string> warnings;
};

class ContactBudgetApplier {
 public:
  static std::vector<ContactCandidate> apply(
      const std::vector<ContactCandidate>& candidates,
      const ContactBudget& budget,
      ContactBudgetStats* stats = nullptr);
};

}  // namespace adasdf
