#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

std::vector<adasdf::ContactCandidate> candidates() {
  std::vector<adasdf::ContactCandidate> output;
  for (int i = 0; i < 10; ++i) {
    adasdf::ContactCandidate c;
    c.sample_id = i;
    c.point = {0.01 * static_cast<double>(i), 0.0, 0.0};
    c.penetration_depth = 1.0 - 0.05 * i;
    c.group_id = i % 2;
    c.has_normal = true;
    c.normal = {1.0, 0.0, 0.0};
    output.push_back(c);
  }
  return output;
}

}  // namespace

int main() {
  adasdf::ContactStabilizationOptions options;
  options.cluster_candidates = false;
  options.budget.enforce_patch_budget = false;
  options.budget.max_contacts_total = 4;
  auto result = adasdf::ContactStabilizer::stabilize(candidates(), options);
  if (result.stabilized_candidates.size() > 4) {
    std::cerr << "max_contacts 4 failed\n";
    return 1;
  }
  options.budget.max_contacts_total = 8;
  result = adasdf::ContactStabilizer::stabilize(candidates(), options);
  if (result.stabilized_candidates.size() > 8 ||
      result.stabilized_candidates.size() > candidates().size()) {
    std::cerr << "max_contacts 8 failed\n";
    return 1;
  }
  options.budget.enforce_patch_budget = true;
  options.budget.max_contacts_per_patch = 1;
  result = adasdf::ContactStabilizer::stabilize(candidates(), options);
  if (result.stabilized_candidates.size() != 2) {
    std::cerr << "patch budget behavior failed\n";
    return 1;
  }
  std::cout << "solver contact budget behavior passed\n";
  return 0;
}
