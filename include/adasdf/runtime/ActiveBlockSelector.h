#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/sparse/CollisionSampleSet.h"

namespace adasdf {

struct ActiveBlockSelectionOptions {
  double threshold = 0.0;
  double selection_band = 0.0;
  double extra_margin = 0.0;
  bool use_sample_radius = true;
  bool include_neighbor_blocks = true;
  bool query_phi_for_selection = true;
  std::size_t max_active_blocks = 0;
};

struct ActiveBlockSelectionResult {
  bool success = false;
  std::string error_message;
  std::vector<int> block_ids;
  std::size_t sample_count = 0;
  std::size_t candidate_sample_count = 0;
  std::size_t candidate_block_count = 0;
  double threshold = 0.0;
  double selection_band = 0.0;
  double extra_margin = 0.0;
  std::vector<std::string> warnings;
};

class ActiveBlockSelector {
 public:
  static ActiveBlockSelectionResult select(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const ActiveBlockSelectionOptions& options =
          ActiveBlockSelectionOptions{});
};

}  // namespace adasdf
