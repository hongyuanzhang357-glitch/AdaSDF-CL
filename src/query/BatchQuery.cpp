#include "adasdf/query/BatchQuery.h"

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

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

void fillStats(
    BatchQueryStats* stats,
    std::size_t num_points,
    double total_ms,
    std::string backend,
    const BatchQueryTiming& timing) {
  if (!stats) {
    return;
  }
  stats->num_points = num_points;
  stats->total_ms = total_ms;
  stats->ns_per_query =
      num_points > 0 ? total_ms * 1.0e6 / static_cast<double>(num_points) : 0.0;
  stats->queries_per_second =
      total_ms > 0.0 ? static_cast<double>(num_points) * 1000.0 / total_ms : 0.0;
  stats->backend = std::move(backend);
  stats->timing = timing;
}

}  // namespace

BatchQueryOutput queryBatchCPU(
    const SDFModel& model,
    const std::vector<Vector3>& points,
    BatchQueryStats* stats,
    BatchQueryTiming* timing) {
  if (!model.isValid() || !model.queryBackendAvailable()) {
    throw std::runtime_error(
        "queryBatchCPU requires a valid SDFModel with query backend support.");
  }

  BatchQueryTiming local_timing;
  const auto allocation0 = std::chrono::steady_clock::now();
  BatchQueryOutput output;
  output.signed_distances.resize(points.size());
  output.gradients.resize(points.size());
  output.normals.resize(points.size());
  const auto allocation1 = std::chrono::steady_clock::now();
  local_timing.allocation_ms =
      std::chrono::duration<double, std::milli>(allocation1 - allocation0).count();

  const auto t0 = std::chrono::steady_clock::now();
  for (std::size_t i = 0; i < points.size(); ++i) {
    output.signed_distances[i] = model.sampleDistance(points[i]);
    output.gradients[i] = model.sampleGradient(points[i]);
    output.normals[i] = normalizedOrFallback(output.gradients[i]);
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double total_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  local_timing.kernel_ms = total_ms;
  local_timing.total_ms =
      std::chrono::duration<double, std::milli>(t1 - allocation0).count();

  std::string backend = "CPU batch query: sampleDistance + sampleGradient";
  if (!model.metadata().query_backend.empty()) {
    backend += " (" + model.metadata().query_backend + ")";
  }
  if (timing != nullptr) {
    *timing = local_timing;
  }
  fillStats(stats, points.size(), local_timing.total_ms, std::move(backend), local_timing);
  return output;
}

}  // namespace adasdf
