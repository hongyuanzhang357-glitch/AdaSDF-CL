#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/sparse/CollisionSampleSet.h"

namespace adasdf {

enum class SparseQueryOutputMode {
  PhiOnly,
  PhiAndNormal
};

struct SparseSDFQueryOptions {
  double threshold = 0.0;
  bool early_exit = false;
  bool compute_normals = false;
  SparseQueryOutputMode output_mode = SparseQueryOutputMode::PhiOnly;
  bool include_non_colliding_samples = true;
  bool use_sample_radius = true;
  bool sort_results_by_effective_phi = false;
};

struct SparseSDFSampleResult {
  int sample_id = -1;
  Vector3 position;
  double radius = 0.0;
  double phi = 0.0;
  double effective_phi = 0.0;
  bool colliding = false;
  bool within_threshold = false;
  Vector3 normal;
  bool has_normal = false;
  int object_id = 0;
  int link_id = 0;
  int group_id = 0;
  std::string label;
};

struct SparseSDFQueryStats {
  std::size_t sample_count = 0;
  std::size_t queried_count = 0;
  std::size_t result_count = 0;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  bool early_exit_triggered = false;
  double elapsed_ms = 0.0;
};

struct SparseSDFQueryResult {
  bool success = false;
  std::string error_message;
  bool colliding = false;
  std::vector<SparseSDFSampleResult> samples;
  SparseSDFQueryStats stats;
  std::vector<std::string> warnings;
};

class SparseSDFQuery {
 public:
  static SparseSDFQueryResult query(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const SparseSDFQueryOptions& options = SparseSDFQueryOptions{});
};

const char* toString(SparseQueryOutputMode mode);

}  // namespace adasdf
