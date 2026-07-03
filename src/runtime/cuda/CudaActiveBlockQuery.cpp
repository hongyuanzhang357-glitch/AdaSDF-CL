#include "adasdf/runtime/cuda/CudaActiveBlockQuery.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>

#include "adasdf/backend/CudaQueryBackend.h"
#include "adasdf/runtime/ActiveBlockSelector.h"
#include "adasdf/runtime/BlockExpansionManager.h"

namespace adasdf {

#ifndef ADASDF_CL_HAS_CUDA_BACKEND
#define ADASDF_CL_HAS_CUDA_BACKEND 0
#endif

#if ADASDF_CL_HAS_CUDA_BACKEND
namespace cuda_detail {
bool queryActiveBlockBufferOnCuda(
    void* buffer_handle,
    const std::vector<Vector3>& points,
    const std::vector<double>& radii,
    double threshold,
    bool compute_normals,
    CudaActiveBlockDeviceQueryResult* output);
}  // namespace cuda_detail
#endif

namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

double norm(const Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 normalizedOrZero(const Vector3& v) {
  const double n = norm(v);
  if (!(n > 1.0e-20) || !std::isfinite(n)) {
    return {};
  }
  return v / n;
}

void sortSamplesAndSources(CudaActiveBlockQueryResult& result) {
  std::vector<std::size_t> order(result.samples.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    const auto& lhs = result.samples[a];
    const auto& rhs = result.samples[b];
    if (lhs.effective_phi != rhs.effective_phi) {
      return lhs.effective_phi < rhs.effective_phi;
    }
    return lhs.sample_id < rhs.sample_id;
  });

  std::vector<SparseSDFSampleResult> samples;
  std::vector<std::string> sources;
  samples.reserve(result.samples.size());
  sources.reserve(result.sample_sources.size());
  for (const std::size_t index : order) {
    samples.push_back(result.samples[index]);
    sources.push_back(
        index < result.sample_sources.size() ? result.sample_sources[index] : "");
  }
  result.samples = std::move(samples);
  result.sample_sources = std::move(sources);
}

}  // namespace

bool CudaActiveBlockQuery::isAvailable() {
  return CudaQueryBackend::isAvailable();
}

CudaActiveBlockQueryResult CudaActiveBlockQuery::query(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const CudaActiveBlockQueryOptions& options) {
  const auto total_start = Clock::now();
  CudaActiveBlockQueryResult result;
  result.stats.sample_count = sample_set.size();
  result.cuda_available = isAvailable();

  if (!result.cuda_available) {
    result.error_message =
        "CUDA active block query unavailable. Configure with "
        "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit and runtime.";
    result.warnings.push_back(result.error_message);
    return result;
  }
  if (!model.isValid() || !model.queryBackendAvailable()) {
    result.error_message =
        "CudaActiveBlockQuery requires a valid queryable SDF model.";
    return result;
  }

  ActiveBlockSelectionOptions selection_options;
  selection_options.threshold = options.threshold;
  selection_options.selection_band = options.selection_band;
  selection_options.extra_margin = options.extra_margin;
  selection_options.use_sample_radius = options.use_sample_radius;
  selection_options.include_neighbor_blocks = options.include_neighbors;
  selection_options.query_phi_for_selection = true;

  const auto selection_start = Clock::now();
  const ActiveBlockSelectionResult selection =
      ActiveBlockSelector::select(model, sample_set, selection_options);
  result.stats.selection_time_ms = elapsedMs(selection_start, Clock::now());
  if (!selection.success) {
    result.error_message = selection.error_message;
    result.warnings = selection.warnings;
    return result;
  }
  result.warnings.insert(
      result.warnings.end(),
      selection.warnings.begin(),
      selection.warnings.end());
  result.stats.active_block_count = selection.block_ids.size();

  CudaActiveBlockCache cache(options.cache_options);
  BlockExpansionManager expansion_manager(&cache.cpuCache());
  const auto expansion_start = Clock::now();
  const BlockExpansionResult expansion =
      expansion_manager.ensureBlocksExpanded(model, selection.block_ids);
  result.stats.cpu_expansion_time_ms =
      elapsedMs(expansion_start, Clock::now());
  if (!expansion.success) {
    result.error_message = expansion.error_message;
    result.warnings.insert(
        result.warnings.end(),
        expansion.warnings.begin(),
        expansion.warnings.end());
    return result;
  }
  result.warnings.insert(
      result.warnings.end(),
      expansion.warnings.begin(),
      expansion.warnings.end());
  result.stats.expanded_block_count = expansion.stats.expanded_block_count;

  const CudaActiveBlockUploadStats upload =
      cache.uploadActiveBlocks(selection.block_ids);
  result.stats.gpu_upload_time_ms = upload.upload_time_ms;
  result.stats.gpu_memory_bytes = upload.total_bytes;
  if (!upload.success) {
    result.error_message = upload.error_message;
    result.warnings.insert(
        result.warnings.end(),
        upload.warnings.begin(),
        upload.warnings.end());
    return result;
  }

  std::vector<Vector3> points;
  std::vector<double> radii;
  points.reserve(sample_set.size());
  radii.reserve(sample_set.size());
  for (const CollisionSample& sample : sample_set.samples) {
    points.push_back(sample.position);
    radii.push_back(options.use_sample_radius ? std::max(0.0, sample.radius) : 0.0);
  }

  const bool compute_normals =
      options.compute_normals ||
      options.output_mode == CudaActiveBlockOutputMode::PhiAndNormal;
  CudaActiveBlockDeviceQueryResult device_result;
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!cuda_detail::queryActiveBlockBufferOnCuda(
          cache.gpuBuffer().deviceHandle(),
          points,
          radii,
          options.threshold,
          compute_normals,
          &device_result)) {
    result.error_message = device_result.error_message.empty()
        ? "CUDA active block kernel query failed."
        : device_result.error_message;
    return result;
  }
#else
  result.error_message =
      "CUDA active block query was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.";
  return result;
#endif
  result.stats.sample_upload_time_ms = device_result.sample_upload_time_ms;
  result.stats.kernel_time_ms = device_result.kernel_time_ms;
  result.stats.download_time_ms = device_result.download_time_ms;

  bool min_initialized = false;
  double min_phi = std::numeric_limits<double>::infinity();
  double min_effective_phi = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < sample_set.samples.size(); ++i) {
    const CollisionSample& sample = sample_set.samples[i];
    SparseSDFSampleResult item;
    item.sample_id = sample.sample_id;
    item.position = sample.position;
    item.radius = i < radii.size() ? radii[i] : 0.0;
    item.object_id = sample.object_id;
    item.link_id = sample.link_id;
    item.group_id = sample.group_id;
    item.label = sample.label;

    std::string source = "cuda";
    const bool fallback_needed =
        i < device_result.fallback_needed.size() &&
        device_result.fallback_needed[i] != 0;
    if (fallback_needed) {
      if (!options.fallback_to_cpu_model_query) {
        result.warnings.push_back(
            "sample skipped because it does not fall in an uploaded active block.");
        continue;
      }
      source = "fallback";
      item.phi = model.sampleDistance(sample.position);
      item.effective_phi = item.phi - item.radius;
      item.colliding = item.effective_phi <= options.threshold;
      item.within_threshold = item.colliding;
      ++result.stats.cpu_fallback_query_count;
      if (compute_normals) {
        item.normal = normalizedOrZero(model.sampleGradient(sample.position));
        item.has_normal = item.normal.allFinite();
      }
    } else {
      item.phi = i < device_result.phi.size() ? device_result.phi[i] : 0.0;
      item.effective_phi = i < device_result.effective_phi.size()
          ? device_result.effective_phi[i]
          : item.phi - item.radius;
      item.colliding =
          i < device_result.colliding.size()
              ? device_result.colliding[i] != 0
              : item.effective_phi <= options.threshold;
      item.within_threshold = item.colliding;
      ++result.stats.gpu_query_count;
      if (compute_normals && i < device_result.normals.size()) {
        item.normal = normalizedOrZero(device_result.normals[i]);
        item.has_normal = item.normal.allFinite();
      }
    }

    if (!std::isfinite(item.phi) || !std::isfinite(item.effective_phi)) {
      result.error_message =
          "CudaActiveBlockQuery produced a non-finite distance for sample " +
          std::to_string(sample.sample_id);
      return result;
    }
    ++result.stats.queried_count;
    min_initialized = true;
    min_phi = std::min(min_phi, item.phi);
    min_effective_phi = std::min(min_effective_phi, item.effective_phi);
    if (item.colliding) {
      result.colliding = true;
      ++result.stats.colliding_count;
    }

    result.samples.push_back(item);
    result.sample_sources.push_back(source);
    if (options.early_exit && item.colliding) {
      result.warnings.push_back(
          "early_exit is applied after the v1.11 CUDA full-query kernel.");
      break;
    }
  }

  if (min_initialized) {
    result.stats.min_phi = min_phi;
    result.stats.min_effective_phi = min_effective_phi;
  }
  result.stats.result_count = result.samples.size();
  result.stats.colliding = result.colliding;
  result.stats.cache_stats = cache.stats();
  result.stats.gpu_memory_bytes = cache.gpuBuffer().deviceMemoryBytes();
  result.stats.total_time_ms = elapsedMs(total_start, Clock::now());
  if (options.sort_results_by_effective_phi) {
    sortSamplesAndSources(result);
  }
  result.success = true;
  return result;
}

}  // namespace adasdf
