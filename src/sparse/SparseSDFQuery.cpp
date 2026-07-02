#include "adasdf/sparse/SparseSDFQuery.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

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

}  // namespace

const char* toString(SparseQueryOutputMode mode) {
  switch (mode) {
    case SparseQueryOutputMode::PhiOnly:
      return "phi-only";
    case SparseQueryOutputMode::PhiAndNormal:
      return "phi-normal";
  }
  return "unknown";
}

SparseSDFQueryResult SparseSDFQuery::query(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const SparseSDFQueryOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  SparseSDFQueryResult result;
  result.stats.sample_count = sample_set.size();

  if (!model.isValid() || !model.queryBackendAvailable()) {
    result.error_message =
        "SparseSDFQuery requires a valid SDFModel with a direct query backend.";
    return result;
  }

  const bool compute_normals =
      options.compute_normals ||
      options.output_mode == SparseQueryOutputMode::PhiAndNormal;
  bool min_initialized = false;
  double min_phi = std::numeric_limits<double>::infinity();
  double min_effective_phi = std::numeric_limits<double>::infinity();

  for (const CollisionSample& sample : sample_set.samples) {
    SparseSDFSampleResult item;
    item.sample_id = sample.sample_id;
    item.position = sample.position;
    item.radius = options.use_sample_radius ? std::max(0.0, sample.radius) : 0.0;
    item.object_id = sample.object_id;
    item.link_id = sample.link_id;
    item.group_id = sample.group_id;
    item.label = sample.label;

    item.phi = model.sampleDistance(sample.position);
    item.effective_phi = item.phi - item.radius;
    item.colliding = item.effective_phi <= options.threshold;
    item.within_threshold = item.colliding;
    ++result.stats.queried_count;

    if (!std::isfinite(item.phi) || !std::isfinite(item.effective_phi)) {
      result.success = false;
      result.error_message =
          "SparseSDFQuery produced a non-finite distance for sample " +
          std::to_string(item.sample_id);
      return result;
    }

    min_initialized = true;
    min_phi = std::min(min_phi, item.phi);
    min_effective_phi = std::min(min_effective_phi, item.effective_phi);
    result.colliding = result.colliding || item.colliding;

    const bool include_sample =
        options.include_non_colliding_samples || item.colliding;
    if (include_sample) {
      if (compute_normals) {
        const Vector3 gradient = model.sampleGradient(sample.position);
        item.normal = normalizedOrZero(gradient);
        item.has_normal = item.normal.allFinite();
      }
      result.samples.push_back(item);
    }

    if (options.early_exit && item.colliding) {
      result.stats.early_exit_triggered = true;
      break;
    }
  }

  if (min_initialized) {
    result.stats.min_phi = min_phi;
    result.stats.min_effective_phi = min_effective_phi;
  }
  if (options.sort_results_by_effective_phi) {
    std::sort(
        result.samples.begin(),
        result.samples.end(),
        [](const SparseSDFSampleResult& a, const SparseSDFSampleResult& b) {
          if (a.effective_phi != b.effective_phi) {
            return a.effective_phi < b.effective_phi;
          }
          return a.sample_id < b.sample_id;
        });
  }
  result.stats.result_count = result.samples.size();
  result.stats.elapsed_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  result.success = true;
  return result;
}

}  // namespace adasdf
