#include <adasdf/adasdf.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(int id, double x, double depth) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.point = {x, 0.0, 0.0};
  c.penetration_depth = depth;
  c.has_normal = true;
  c.normal = {1.0, 0.0, 0.0};
  return c;
}

std::vector<int> ids(const adasdf::SolverContactSet& contacts) {
  std::vector<int> values;
  for (const auto& contact : contacts.contacts) {
    values.push_back(contact.sample_id);
  }
  return values;
}

}  // namespace

int main() {
  std::vector<adasdf::ContactCandidate> input = {
      candidate(3, 0.03, 0.3),
      candidate(1, 0.00, 0.5),
      candidate(2, 0.01, 0.4)};
  adasdf::ContactStabilizationOptions options;
  options.patch_options.spatial_radius = 0.005;
  auto a = adasdf::ContactStabilizer::stabilize(input, options);
  std::reverse(input.begin(), input.end());
  auto b = adasdf::ContactStabilizer::stabilize(input, options);
  auto ca = adasdf::SolverContactBuilder::fromCandidates(
      a.stabilized_candidates,
      a.patches);
  auto cb = adasdf::SolverContactBuilder::fromCandidates(
      b.stabilized_candidates,
      b.patches);
  if (ids(ca) != ids(cb) ||
      ca.contacts.front().stable_key != cb.contacts.front().stable_key) {
    std::cerr << "contact determinism failed\n";
    return 1;
  }
  std::cout << "contact determinism passed\n";
  return 0;
}
