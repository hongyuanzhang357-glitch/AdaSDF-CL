#include "adasdf/query/QueryEngine.h"

#include <chrono>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "adasdf/backend/CudaQueryBackend.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

double norm(const Vector3& value) {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vector3 normalizedOrFallback(const Vector3& value) {
  const double length = norm(value);
  if (!(length > 1.0e-12) || !value.allFinite()) {
    return {1.0, 0.0, 0.0};
  }
  return value / length;
}

QueryExpansionMode normalizedExpansion(const QueryModeConfig& config) {
  if (config.expansion != QueryExpansionMode::Auto) {
    return config.expansion;
  }
  return config.backend == QueryBackend::CUDA ? QueryExpansionMode::Global
                                              : QueryExpansionMode::None;
}

}  // namespace

QueryEngine::QueryEngine(std::shared_ptr<SDFModel> model, QueryModeConfig config)
    : QueryEngine(
          std::move(model),
          std::move(config),
          ExpansionOptions{}) {}

QueryEngine::QueryEngine(
    std::shared_ptr<SDFModel> model,
    QueryModeConfig config,
    ExpansionOptions expansion_options)
    : model_(std::move(model)),
      config_(std::move(config)),
      expansion_options_(std::move(expansion_options)) {
  if (config_.expansion != QueryExpansionMode::Auto) {
    expansion_options_.expansion = config_.expansion;
  }
  expansion_options_.block_selection = config_.block_selection;
}

QueryEngine::~QueryEngine() = default;
QueryEngine::QueryEngine(QueryEngine&&) noexcept = default;
QueryEngine& QueryEngine::operator=(QueryEngine&&) noexcept = default;

bool QueryEngine::prepare() {
  if (!model_) {
    throw std::runtime_error("QueryEngine requires a non-null SDFModel.");
  }
  validateQueryModeConfig(config_);
  execution_mode_ = resolveQueryExecutionMode(config_);
  stats_ = {};
  stats_.backend = toString(config_.backend);
  stats_.expansion_mode = toString(normalizedExpansion(config_));

  const auto t0 = Clock::now();
  bool ok = false;
  switch (execution_mode_) {
    case QueryExecutionMode::DirectCPU:
      ok = true;
      break;
    case QueryExecutionMode::ExpandedCPU:
      ok = prepareCPUExpanded();
      break;
    case QueryExecutionMode::ExpandedCUDA:
      ok = prepareCUDAExpanded();
      break;
  }
  stats_.setup_ms = elapsedMs(t0, Clock::now());
  stats_.timing.setup_ms = stats_.setup_ms;
  prepared_ = ok;
  return ok;
}

bool QueryEngine::prepareCPUExpanded() {
  expanded_ = SDFExpander::expand(*model_, expansion_options_);
  stats_.expanded_memory_bytes = expanded_.memoryFootprintBytes();
  stats_.backend = "cpu";
  cuda_active_ = false;
  return true;
}

bool QueryEngine::prepareCUDAExpanded() {
  if (!CudaQueryBackend::isAvailable()) {
    if (!config_.allow_fallback_to_cpu) {
      stats_.backend = "cuda_unavailable";
      return false;
    }
    expanded_ = SDFExpander::expand(*model_, expansion_options_);
    stats_.expanded_memory_bytes = expanded_.memoryFootprintBytes();
    stats_.backend = "cpu_fallback_cuda_unavailable";
    cuda_active_ = false;
    return true;
  }

  expanded_ = SDFExpander::expand(*model_, expansion_options_);
  stats_.expanded_memory_bytes = expanded_.memoryFootprintBytes();
  resident_ = std::make_unique<CudaResidentExpandedSDF>();
  if (!resident_->upload(expanded_)) {
    if (!config_.allow_fallback_to_cpu) {
      stats_.backend = "cuda_upload_failed";
      return false;
    }
    resident_.reset();
    stats_.backend = "cpu_fallback_cuda_upload_failed";
    cuda_active_ = false;
    return true;
  }
  stats_.backend = "cuda";
  stats_.gpu_resident_memory_bytes = resident_->deviceMemoryBytes();
  cuda_active_ = true;
  return true;
}

void QueryEngine::ensurePrepared() {
  if (!prepared_ && !prepare()) {
    throw std::runtime_error(
        "QueryEngine prepare() failed for backend=" + std::string(toString(config_.backend)) +
        " expansion=" + std::string(toString(config_.expansion)));
  }
}

double QueryEngine::sampleDistance(const Vector3& p) {
  ensurePrepared();
  if (execution_mode_ == QueryExecutionMode::DirectCPU) {
    return model_->sampleDistance(p);
  }
  try {
    return expanded_.sampleDistance(p);
  } catch (const std::exception&) {
    if (!config_.allow_fallback_to_cpu ||
        !expansion_options_.allow_direct_fallback_outside) {
      throw;
    }
    ++stats_.fallback_count;
    return model_->sampleDistance(p);
  }
}

Vector3 QueryEngine::sampleGradient(const Vector3& p) {
  ensurePrepared();
  if (execution_mode_ == QueryExecutionMode::DirectCPU) {
    return model_->sampleGradient(p);
  }
  try {
    return expanded_.sampleGradient(p);
  } catch (const std::exception&) {
    if (!config_.allow_fallback_to_cpu ||
        !expansion_options_.allow_direct_fallback_outside) {
      throw;
    }
    ++stats_.fallback_count;
    return model_->sampleGradient(p);
  }
}

BatchQueryOutput QueryEngine::queryBatch(const std::vector<Vector3>& points) {
  ensurePrepared();
  const auto t0 = Clock::now();
  BatchQueryOutput output;

  if (cuda_active_ && resident_) {
    BatchQueryTiming timing;
    output = resident_->queryBatch(points, false, nullptr, &timing);
    timing.setup_ms = stats_.setup_ms;
    stats_.query_kernel_ms = resident_->lastKernelMs();
    stats_.query_total_ms = resident_->lastQueryMs();
    stats_.timing = timing;
  } else {
    const auto allocation0 = Clock::now();
    output.signed_distances.resize(points.size());
    output.gradients.resize(points.size());
    output.normals.resize(points.size());
    const auto allocation1 = Clock::now();
    for (std::size_t i = 0; i < points.size(); ++i) {
      output.signed_distances[i] = sampleDistance(points[i]);
      output.gradients[i] = sampleGradient(points[i]);
      output.normals[i] = normalizedOrFallback(output.gradients[i]);
    }
    stats_.query_total_ms = elapsedMs(t0, Clock::now());
    stats_.query_kernel_ms = 0.0;
    stats_.timing = {};
    stats_.timing.setup_ms = stats_.setup_ms;
    stats_.timing.allocation_ms = elapsedMs(allocation0, allocation1);
    stats_.timing.total_ms = stats_.query_total_ms;
  }

  stats_.num_queries += points.size();
  stats_.total_ms += elapsedMs(t0, Clock::now());
  if (stats_.query_total_ms <= 0.0) {
    stats_.query_total_ms = stats_.total_ms;
  }
  return output;
}

std::string QueryEngine::description() const {
  std::ostringstream out;
  out << "QueryEngine backend=" << stats_.backend
      << " expansion=" << stats_.expansion_mode
      << " execution=" << toString(execution_mode_)
      << " expanded_memory_bytes=" << stats_.expanded_memory_bytes
      << " gpu_resident_memory_bytes=" << stats_.gpu_resident_memory_bytes;
  return out.str();
}

}  // namespace adasdf
