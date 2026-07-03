#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(int id, double x, double depth) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.point = {x, 0.0, 0.0};
  c.penetration_depth = depth;
  c.phi = -depth;
  c.effective_phi = -depth;
  c.has_normal = true;
  c.normal = {1.0, 0.0, 0.0};
  return c;
}

}  // namespace

int main() {
  std::vector<adasdf::ContactCandidate> input = {
      candidate(2, 0.0, 0.2),
      candidate(2, 0.0, 0.5),
      candidate(3, 0.01, 0.4),
      candidate(4, 1.0, 0.01)};
  adasdf::ContactStabilizationOptions options;
  options.min_penetration_depth = 0.05;
  options.patch_options.spatial_radius = 0.05;
  options.budget.max_contacts_total = 1;
  auto result = adasdf::ContactStabilizer::stabilize(input, options);
  if (!result.success ||
      result.stats.after_duplicate_removal_count != 3 ||
      result.stats.after_threshold_count != 2 ||
      result.stats.patch_count != 1 ||
      result.stabilized_candidates.size() != 1 ||
      result.stabilized_candidates[0].sample_id != 2) {
    std::cerr << "contact stabilizer pipeline failed\n";
    return 1;
  }
  auto again = adasdf::ContactStabilizer::stabilize(input, options);
  if (again.stabilized_candidates[0].sample_id !=
      result.stabilized_candidates[0].sample_id) {
    std::cerr << "contact stabilizer determinism failed\n";
    return 1;
  }
  std::cout << "contact stabilizer passed\n";
  return 0;
}
