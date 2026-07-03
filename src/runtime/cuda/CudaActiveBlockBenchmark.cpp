#include "adasdf/runtime/cuda/CudaActiveBlockBenchmark.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

#include "adasdf/runtime/ActiveBlockQuery.h"
#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

CudaActiveBlockBenchmarkResult CudaActiveBlockBenchmark::run(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const CudaActiveBlockBenchmarkOptions& options) {
  CudaActiveBlockBenchmarkResult result;
  result.sample_count = sample_set.size();
  result.cuda_available = CudaActiveBlockQuery::isAvailable();
  if (!result.cuda_available) {
    result.error_message =
        "CUDA active block benchmark skipped because CUDA is unavailable.";
    result.warnings.push_back(result.error_message);
    return result;
  }

  const int warmup = std::max(0, options.warmup);
  const int repeat = std::max(1, options.repeat);
  CudaActiveBlockQueryOptions query_options = options.query_options;
  query_options.threshold = options.threshold;
  query_options.selection_band = options.selection_band;
  query_options.extra_margin = options.extra_margin;
  query_options.compute_normals = options.with_normal;
  query_options.output_mode = options.with_normal
      ? CudaActiveBlockOutputMode::PhiAndNormal
      : CudaActiveBlockOutputMode::PhiOnly;

  auto run_cuda_once = [&]() {
    CudaActiveBlockQueryResult query =
        CudaActiveBlockQuery::query(model, sample_set, query_options);
    if (!query.success) {
      throw std::runtime_error(query.error_message);
    }
    return query;
  };

  try {
    for (int i = 0; i < warmup; ++i) {
      (void)run_cuda_once();
    }
    double total_ms = 0.0;
    double kernel_ms = 0.0;
    double upload_ms = 0.0;
    double download_ms = 0.0;
    for (int i = 0; i < repeat; ++i) {
      const CudaActiveBlockQueryResult query = run_cuda_once();
      total_ms += query.stats.total_time_ms;
      kernel_ms += query.stats.kernel_time_ms;
      upload_ms += query.stats.gpu_upload_time_ms + query.stats.sample_upload_time_ms;
      download_ms += query.stats.download_time_ms;
      result.active_block_count = query.stats.active_block_count;
      result.gpu_memory_bytes = query.stats.gpu_memory_bytes;
    }
    result.cuda_total_avg_ms = total_ms / static_cast<double>(repeat);
    result.cuda_kernel_avg_ms = kernel_ms / static_cast<double>(repeat);
    result.cuda_upload_avg_ms = upload_ms / static_cast<double>(repeat);
    result.cuda_download_avg_ms = download_ms / static_cast<double>(repeat);
    result.cuda_ns_per_sample = result.sample_count > 0
        ? result.cuda_total_avg_ms * 1.0e6 /
              static_cast<double>(result.sample_count)
        : 0.0;
  } catch (const std::exception& exc) {
    result.error_message = exc.what();
    return result;
  }

  if (options.compare_cpu_active_block) {
    ActiveBlockSelectionOptions selection_options;
    selection_options.threshold = options.threshold;
    selection_options.selection_band = options.selection_band;
    selection_options.extra_margin = options.extra_margin;
    ExpandedBlockCache cache;
    ActiveBlockQueryOptions active_options;
    active_options.threshold = options.threshold;
    active_options.compute_normals = options.with_normal;
    active_options.output_mode = options.with_normal
        ? SparseQueryOutputMode::PhiAndNormal
        : SparseQueryOutputMode::PhiOnly;
    auto run_cpu_active_once = [&]() {
      const ActiveBlockQueryResult query =
          ActiveBlockQuery::queryWithSelection(
              model, sample_set, cache, selection_options, active_options);
      if (!query.success) {
        throw std::runtime_error(query.error_message);
      }
    };
    for (int i = 0; i < warmup; ++i) {
      run_cpu_active_once();
    }
    const auto start = Clock::now();
    for (int i = 0; i < repeat; ++i) {
      run_cpu_active_once();
    }
    result.cpu_active_avg_ms =
        elapsedMs(start, Clock::now()) / static_cast<double>(repeat);
  }

  if (options.compare_direct_sparse) {
    SparseSDFQueryOptions direct_options;
    direct_options.threshold = options.threshold;
    direct_options.compute_normals = options.with_normal;
    direct_options.output_mode = options.with_normal
        ? SparseQueryOutputMode::PhiAndNormal
        : SparseQueryOutputMode::PhiOnly;
    auto run_direct_once = [&]() {
      const SparseSDFQueryResult query =
          SparseSDFQuery::query(model, sample_set, direct_options);
      if (!query.success) {
        throw std::runtime_error(query.error_message);
      }
    };
    for (int i = 0; i < warmup; ++i) {
      run_direct_once();
    }
    const auto start = Clock::now();
    for (int i = 0; i < repeat; ++i) {
      run_direct_once();
    }
    result.direct_sparse_avg_ms =
        elapsedMs(start, Clock::now()) / static_cast<double>(repeat);
  }

  if (options.compare_global_expanded) {
    result.warnings.push_back(
        "global expanded comparison is not implemented by the v1.11 CUDA "
        "active block benchmark.");
  }

  result.success = true;
  return result;
}

}  // namespace adasdf
