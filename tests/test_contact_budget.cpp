#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(int id, double depth, int link, int group) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.penetration_depth = depth;
  c.link_id = link;
  c.group_id = group;
  c.has_normal = true;
  c.normal = {1.0, 0.0, 0.0};
  return c;
}

}  // namespace

int main() {
  std::vector<adasdf::ContactCandidate> input = {
      candidate(3, 0.1, 1, 0),
      candidate(1, 0.5, 1, 0),
      candidate(2, 0.4, 2, 0),
      candidate(4, 0.3, 2, 1)};

  adasdf::ContactBudget budget;
  budget.max_contacts_total = 2;
  budget.enforce_patch_budget = false;
  adasdf::ContactBudgetStats stats;
  auto output = adasdf::ContactBudgetApplier::apply(input, budget, &stats);
  if (output.size() != 2 || output[0].sample_id != 1 || output[1].sample_id != 2 ||
      stats.after_total_budget_count != 2) {
    std::cerr << "total contact budget failed\n";
    return 1;
  }

  budget.max_contacts_total = 8;
  budget.enforce_link_budget = true;
  budget.max_contacts_per_link = 1;
  output = adasdf::ContactBudgetApplier::apply(input, budget, &stats);
  if (output.size() != 2 || output[0].link_id == output[1].link_id) {
    std::cerr << "link contact budget failed\n";
    return 1;
  }

  budget.enforce_link_budget = false;
  budget.enforce_patch_budget = true;
  budget.max_contacts_per_patch = 1;
  output = adasdf::ContactBudgetApplier::apply(input, budget, &stats);
  if (output.size() != 2 || output[0].group_id == output[1].group_id) {
    std::cerr << "patch contact budget failed\n";
    return 1;
  }

  budget.max_contacts_total = 0;
  output = adasdf::ContactBudgetApplier::apply(input, budget, &stats);
  if (!output.empty() || stats.warnings.empty()) {
    std::cerr << "zero budget warning failed\n";
    return 1;
  }

  std::cout << "contact budget passed\n";
  return 0;
}
