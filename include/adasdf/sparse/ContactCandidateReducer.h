#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {

struct ContactCandidateOptions {
  int top_k = 8;
  double candidate_threshold = 0.0;
  double reduction_radius = 0.0;
  bool prefer_deeper_penetration = true;
  bool compute_normals = true;
  bool deterministic_sort = true;
};

struct ContactCandidate {
  int rank = -1;
  int sample_id = -1;
  Vector3 point;
  Vector3 normal;
  bool has_normal = false;
  double phi = 0.0;
  double effective_phi = 0.0;
  double penetration_depth = 0.0;
  double radius = 0.0;
  int object_id = 0;
  int link_id = 0;
  int group_id = 0;
  std::string label;
};

struct ContactCandidateReductionResult {
  std::vector<ContactCandidate> candidates;
  std::size_t input_count = 0;
  std::size_t threshold_candidate_count = 0;
  std::size_t reduced_count = 0;
  std::vector<std::string> warnings;
};

class ContactCandidateReducer {
 public:
  static ContactCandidateReductionResult reduce(
      const std::vector<SparseSDFSampleResult>& sparse_results,
      const ContactCandidateOptions& options = ContactCandidateOptions{});
};

}  // namespace adasdf
