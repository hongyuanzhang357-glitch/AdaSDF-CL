#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {

enum class SparseCollisionMode {
  CollisionOnly,
  Clearance,
  CandidateSearch
};

struct SparseCollisionQueryOptions {
  SparseCollisionMode mode = SparseCollisionMode::CollisionOnly;
  double threshold = 0.0;
  bool early_exit = true;
  bool compute_normals = false;
  bool use_sample_radius = true;
  bool return_all_violations = false;
};

struct SparseCollisionResult {
  bool success = false;
  std::string error_message;
  bool colliding = false;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  int first_hit_sample_id = -1;
  std::size_t sample_count = 0;
  std::size_t queried_count = 0;
  bool early_exit_triggered = false;
  std::vector<SparseSDFSampleResult> violations;
  double elapsed_ms = 0.0;
  std::vector<std::string> warnings;
};

class SparseCollisionQuery {
 public:
  static SparseCollisionResult check(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const SparseCollisionQueryOptions& options = SparseCollisionQueryOptions{});
};

const char* toString(SparseCollisionMode mode);
SparseCollisionMode sparseCollisionModeFromString(const std::string& text);

}  // namespace adasdf
