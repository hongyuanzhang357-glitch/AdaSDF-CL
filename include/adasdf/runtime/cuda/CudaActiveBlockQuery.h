#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/runtime/cuda/CudaActiveBlockCache.h"
#include "adasdf/sparse/CollisionSampleSet.h"
#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {

enum class CudaActiveBlockOutputMode {
  PhiOnly,
  PhiAndNormal
};

struct CudaActiveBlockQueryOptions {
  double threshold = 0.0;
  double selection_band = 0.0;
  double extra_margin = 0.0;
  bool use_sample_radius = true;
  bool include_neighbors = false;
  bool compute_normals = false;
  CudaActiveBlockOutputMode output_mode = CudaActiveBlockOutputMode::PhiOnly;
  bool fallback_to_cpu_model_query = true;
  bool early_exit = false;
  bool sort_results_by_effective_phi = false;
  CudaActiveBlockCacheOptions cache_options;
};

struct CudaActiveBlockQueryStats {
  std::size_t sample_count = 0;
  std::size_t queried_count = 0;
  std::size_t result_count = 0;
  std::size_t active_block_count = 0;
  std::size_t expanded_block_count = 0;
  std::size_t gpu_query_count = 0;
  std::size_t cpu_fallback_query_count = 0;
  std::size_t colliding_count = 0;
  bool colliding = false;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  double selection_time_ms = 0.0;
  double cpu_expansion_time_ms = 0.0;
  double gpu_upload_time_ms = 0.0;
  double sample_upload_time_ms = 0.0;
  double kernel_time_ms = 0.0;
  double download_time_ms = 0.0;
  double total_time_ms = 0.0;
  std::size_t gpu_memory_bytes = 0;
  CudaActiveBlockCacheStats cache_stats;
};

struct CudaActiveBlockQueryResult {
  bool success = false;
  std::string error_message;
  bool cuda_available = false;
  bool colliding = false;
  std::vector<SparseSDFSampleResult> samples;
  std::vector<std::string> sample_sources;
  CudaActiveBlockQueryStats stats;
  std::vector<std::string> warnings;
};

class CudaActiveBlockQuery {
 public:
  static bool isAvailable();

  static CudaActiveBlockQueryResult query(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const CudaActiveBlockQueryOptions& options =
          CudaActiveBlockQueryOptions{});
};

}  // namespace adasdf
