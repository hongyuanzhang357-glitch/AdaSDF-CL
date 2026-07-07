#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/runtime/ActiveBlockSelector.h"
#include "adasdf/runtime/BlockExpansionManager.h"
#include "adasdf/lookup/ActiveBlockHashMap.h"
#include "adasdf/lookup/BlockLookupIndex.h"
#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {

struct ActiveBlockQueryOptions {
  double threshold = 0.0;
  bool early_exit = false;
  bool compute_normals = false;
  SparseQueryOutputMode output_mode = SparseQueryOutputMode::PhiOnly;
  bool include_non_colliding_samples = true;
  bool use_sample_radius = true;
  bool fallback_to_model_query = true;
  bool sort_results_by_effective_phi = false;
  BlockLookupMode lookup_mode = BlockLookupMode::SpatialHash;
  BlockLookupMode cache_lookup_mode = BlockLookupMode::SpatialHash;
  bool allow_linear_fallback = true;
  bool report_lookup_stats = false;
};

struct ActiveBlockQueryStats {
  std::size_t sample_count = 0;
  std::size_t queried_count = 0;
  std::size_t result_count = 0;
  std::size_t cache_query_count = 0;
  std::size_t fallback_query_count = 0;
  std::size_t colliding_count = 0;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  bool early_exit_triggered = false;
  double query_time_ms = 0.0;
  BlockExpansionStats expansion_stats;
  ExpandedBlockCacheStats cache_stats;
  BlockLookupIndexStats lookup_stats;
  ActiveBlockHashMapStats cache_lookup_stats;
};

struct ActiveBlockQueryResult {
  bool success = false;
  std::string error_message;
  bool colliding = false;
  std::vector<SparseSDFSampleResult> samples;
  std::vector<std::string> sample_sources;
  ActiveBlockQueryStats stats;
  std::vector<std::string> warnings;
};

class ActiveBlockQuery {
 public:
  static ActiveBlockQueryResult query(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const std::vector<int>& active_block_ids,
      ExpandedBlockCache& cache,
      const ActiveBlockQueryOptions& options = ActiveBlockQueryOptions{});

  static ActiveBlockQueryResult queryWithSelection(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      ExpandedBlockCache& cache,
      const ActiveBlockSelectionOptions& selection_options =
          ActiveBlockSelectionOptions{},
      const ActiveBlockQueryOptions& query_options =
          ActiveBlockQueryOptions{});
};

}  // namespace adasdf
