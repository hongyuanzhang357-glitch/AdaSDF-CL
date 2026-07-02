#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

adasdf::SparseSDFSampleResult sample(int id, double x, double effective_phi) {
  adasdf::SparseSDFSampleResult s;
  s.sample_id = id;
  s.position = {x, 0.0, 0.0};
  s.phi = effective_phi;
  s.effective_phi = effective_phi;
  s.colliding = effective_phi <= 0.0;
  s.has_normal = true;
  s.normal = {1.0, 0.0, 0.0};
  return s;
}

}  // namespace

int main() {
  std::vector<adasdf::SparseSDFSampleResult> samples = {
      sample(2, 0.0, -0.2),
      sample(1, 0.01, -0.2),
      sample(3, 1.0, -0.05),
      sample(4, 2.0, 0.1)};
  adasdf::ContactCandidateOptions options;
  options.top_k = 3;
  options.candidate_threshold = 0.0;
  auto reduced = adasdf::ContactCandidateReducer::reduce(samples, options);
  if (reduced.threshold_candidate_count != 3 || reduced.reduced_count != 3) {
    std::cerr << "threshold/top-k reduction failed\n";
    return 1;
  }
  if (reduced.candidates[0].sample_id != 1 ||
      reduced.candidates[1].sample_id != 2) {
    std::cerr << "deterministic tie by sample_id failed\n";
    return 1;
  }
  if (!(reduced.candidates[0].penetration_depth > 0.0)) {
    std::cerr << "penetration depth should be positive\n";
    return 1;
  }
  options.reduction_radius = 0.05;
  reduced = adasdf::ContactCandidateReducer::reduce(samples, options);
  if (reduced.reduced_count != 2) {
    std::cerr << "reduction radius failed\n";
    return 1;
  }
  std::cout << "contact candidate reducer passed\n";
  return 0;
}
