#include "adasdf/contact/ContactBudget.h"

#include <algorithm>
#include <map>

namespace adasdf {
namespace {

bool deeperFirst(const ContactCandidate& a, const ContactCandidate& b) {
  if (a.penetration_depth != b.penetration_depth) {
    return a.penetration_depth > b.penetration_depth;
  }
  return a.sample_id < b.sample_id;
}

std::vector<ContactCandidate> takeByBucket(
    const std::vector<ContactCandidate>& input,
    int limit,
    bool enabled,
    int ContactCandidate::*bucket,
    const char* warning,
    ContactBudgetStats& stats) {
  if (!enabled) {
    return input;
  }
  if (limit <= 0) {
    stats.warnings.push_back(warning);
    return {};
  }
  std::map<int, int> counts;
  std::vector<ContactCandidate> output;
  for (const ContactCandidate& candidate : input) {
    const int key = candidate.*bucket;
    int& count = counts[key];
    if (count >= limit) {
      continue;
    }
    output.push_back(candidate);
    ++count;
  }
  return output;
}

}  // namespace

std::vector<ContactCandidate> ContactBudgetApplier::apply(
    const std::vector<ContactCandidate>& candidates,
    const ContactBudget& budget,
    ContactBudgetStats* stats) {
  ContactBudgetStats local_stats;
  local_stats.input_candidate_count = candidates.size();

  std::vector<ContactCandidate> sorted = candidates;
  std::sort(sorted.begin(), sorted.end(), deeperFirst);

  std::vector<ContactCandidate> after_total = sorted;
  if (budget.enforce_total_budget) {
    if (budget.max_contacts_total <= 0) {
      local_stats.warnings.push_back(
          "max_contacts_total <= 0; no contacts retained");
      after_total.clear();
    } else if (static_cast<int>(after_total.size()) > budget.max_contacts_total) {
      after_total.resize(static_cast<std::size_t>(budget.max_contacts_total));
    }
  }
  local_stats.after_total_budget_count = after_total.size();

  std::vector<ContactCandidate> after_link = takeByBucket(
      after_total,
      budget.max_contacts_per_link,
      budget.enforce_link_budget,
      &ContactCandidate::link_id,
      "max_contacts_per_link <= 0; link budget removed all contacts",
      local_stats);
  local_stats.after_link_budget_count = after_link.size();

  std::vector<ContactCandidate> after_patch = takeByBucket(
      after_link,
      budget.max_contacts_per_patch,
      budget.enforce_patch_budget,
      &ContactCandidate::group_id,
      "max_contacts_per_patch <= 0; patch budget removed all contacts",
      local_stats);
  local_stats.after_patch_budget_count = after_patch.size();

  std::sort(after_patch.begin(), after_patch.end(), deeperFirst);
  if (stats) {
    *stats = local_stats;
  }
  return after_patch;
}

}  // namespace adasdf
