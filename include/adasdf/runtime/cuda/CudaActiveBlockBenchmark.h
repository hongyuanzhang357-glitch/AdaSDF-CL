#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/runtime/cuda/CudaActiveBlockQuery.h"
#include "adasdf/sparse/CollisionSampleSet.h"

namespace adasdf {

struct CudaActiveBlockBenchmarkOptions {
  int warmup = 5;
  int repeat = 20;
  double threshold = 0.0;
  double selection_band = 0.0;
  double extra_margin = 0.0;
  bool with_normal = false;
  bool compare_cpu_active_block = true;
  bool compare_direct_sparse = true;
  bool compare_global_expanded = false;
  CudaActiveBlockQueryOptions query_options;
};

struct CudaActiveBlockBenchmarkResult {
  bool success = false;
  std::string error_message;
  bool cuda_available = false;
  std::size_t sample_count = 0;
  std::size_t active_block_count = 0;
  std::size_t gpu_memory_bytes = 0;
  double cuda_total_avg_ms = 0.0;
  double cuda_kernel_avg_ms = 0.0;
  double cuda_upload_avg_ms = 0.0;
  double cuda_download_avg_ms = 0.0;
  double cuda_ns_per_sample = 0.0;
  double cpu_active_avg_ms = 0.0;
  double direct_sparse_avg_ms = 0.0;
  double global_expanded_avg_ms = 0.0;
  std::vector<std::string> warnings;
};

class CudaActiveBlockBenchmark {
 public:
  static CudaActiveBlockBenchmarkResult run(
      const SDFModel& model,
      const CollisionSampleSet& sample_set,
      const CudaActiveBlockBenchmarkOptions& options =
          CudaActiveBlockBenchmarkOptions{});
};

}  // namespace adasdf
