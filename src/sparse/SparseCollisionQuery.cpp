#include "adasdf/sparse/SparseCollisionQuery.h"

#include <algorithm>
#include <stdexcept>

namespace adasdf {

const char* toString(SparseCollisionMode mode) {
  switch (mode) {
    case SparseCollisionMode::CollisionOnly:
      return "collision-only";
    case SparseCollisionMode::Clearance:
      return "clearance";
    case SparseCollisionMode::CandidateSearch:
      return "candidate-search";
  }
  return "unknown";
}

SparseCollisionMode sparseCollisionModeFromString(const std::string& text) {
  if (text == "collision-only" || text == "collision") {
    return SparseCollisionMode::CollisionOnly;
  }
  if (text == "clearance") {
    return SparseCollisionMode::Clearance;
  }
  if (text == "candidate-search" || text == "candidates") {
    return SparseCollisionMode::CandidateSearch;
  }
  throw std::runtime_error(
      "sparse collision mode must be collision-only, clearance, or candidate-search");
}

SparseCollisionResult SparseCollisionQuery::check(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const SparseCollisionQueryOptions& options) {
  SparseSDFQueryOptions query_options;
  query_options.threshold = options.threshold;
  query_options.compute_normals = options.compute_normals;
  query_options.output_mode = options.compute_normals
      ? SparseQueryOutputMode::PhiAndNormal
      : SparseQueryOutputMode::PhiOnly;
  query_options.use_sample_radius = options.use_sample_radius;
  query_options.include_non_colliding_samples = false;
  query_options.sort_results_by_effective_phi =
      options.mode == SparseCollisionMode::CandidateSearch ||
      options.return_all_violations;
  query_options.early_exit =
      options.mode == SparseCollisionMode::CollisionOnly && options.early_exit;

  if (options.mode == SparseCollisionMode::Clearance ||
      options.mode == SparseCollisionMode::CandidateSearch) {
    query_options.early_exit = false;
  }

  const SparseSDFQueryResult query =
      SparseSDFQuery::query(model, sample_set, query_options);

  SparseCollisionResult result;
  result.success = query.success;
  result.error_message = query.error_message;
  result.colliding = query.colliding;
  result.min_phi = query.stats.min_phi;
  result.min_effective_phi = query.stats.min_effective_phi;
  result.sample_count = query.stats.sample_count;
  result.queried_count = query.stats.queried_count;
  result.early_exit_triggered = query.stats.early_exit_triggered;
  result.elapsed_ms = query.stats.elapsed_ms;
  result.warnings = query.warnings;

  if (!query.samples.empty()) {
    result.first_hit_sample_id = query.samples.front().sample_id;
  }
  if (options.mode == SparseCollisionMode::CandidateSearch ||
      options.return_all_violations) {
    result.violations = query.samples;
  } else if (query_options.early_exit && !query.samples.empty()) {
    result.violations.push_back(query.samples.front());
  }
  return result;
}

}  // namespace adasdf
