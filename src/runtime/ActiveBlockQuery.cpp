#include "adasdf/runtime/ActiveBlockQuery.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>

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

double diagonalLength(const AABB& box) {
  const Vector3 d = box.max - box.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

const ActiveExpandedBlock* findContainingExpandedBlock(
    const Vector3& point,
    ExpandedBlockCache& cache,
    const std::vector<int>& resident_ids) {
  const ActiveExpandedBlock* best = nullptr;
  int best_level = std::numeric_limits<int>::min();
  double best_diag = std::numeric_limits<double>::infinity();
  for (const int block_id : resident_ids) {
    const ActiveExpandedBlock* block = cache.get(block_id);
    if (!block || !block->contains(point)) {
      continue;
    }
    const double diag = diagonalLength(block->bounds);
    if (block->level > best_level ||
        (block->level == best_level && diag < best_diag) ||
        (block->level == best_level && diag == best_diag &&
         best && block->block_id < best->block_id)) {
      best = block;
      best_level = block->level;
      best_diag = diag;
    }
  }
  return best;
}

void sortSamplesAndSources(ActiveBlockQueryResult& result) {
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

  std::vector<SparseSDFSampleResult> sorted_samples;
  std::vector<std::string> sorted_sources;
  sorted_samples.reserve(result.samples.size());
  sorted_sources.reserve(result.sample_sources.size());
  for (const std::size_t index : order) {
    sorted_samples.push_back(result.samples[index]);
    sorted_sources.push_back(
        index < result.sample_sources.size() ? result.sample_sources[index] : "");
  }
  result.samples = std::move(sorted_samples);
  result.sample_sources = std::move(sorted_sources);
}

}  // namespace

ActiveBlockQueryResult ActiveBlockQuery::query(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const std::vector<int>& active_block_ids,
    ExpandedBlockCache& cache,
    const ActiveBlockQueryOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  ActiveBlockQueryResult result;
  result.stats.sample_count = sample_set.size();

  if (!model.isValid() || !model.queryBackendAvailable()) {
    result.error_message =
        "ActiveBlockQuery requires a valid queryable SDF model.";
    return result;
  }

  BlockExpansionManager manager(&cache);
  const BlockExpansionResult expansion =
      manager.ensureBlocksExpanded(model, active_block_ids);
  if (!expansion.success) {
    result.error_message = expansion.error_message;
    result.warnings = expansion.warnings;
    return result;
  }
  result.warnings = expansion.warnings;
  result.stats.expansion_stats = expansion.stats;

  const bool compute_normals =
      options.compute_normals ||
      options.output_mode == SparseQueryOutputMode::PhiAndNormal;
  const std::vector<int> resident_ids = cache.residentBlockIds();
  bool min_initialized = false;
  double min_phi = std::numeric_limits<double>::infinity();
  double min_effective_phi = std::numeric_limits<double>::infinity();

  for (const CollisionSample& sample : sample_set.samples) {
    SparseSDFSampleResult item;
    item.sample_id = sample.sample_id;
    item.position = sample.position;
    item.radius =
        options.use_sample_radius ? std::max(0.0, sample.radius) : 0.0;
    item.object_id = sample.object_id;
    item.link_id = sample.link_id;
    item.group_id = sample.group_id;
    item.label = sample.label;

    const ActiveExpandedBlock* block =
        findContainingExpandedBlock(sample.position, cache, resident_ids);
    std::string source = "cache";
    if (block) {
      item.phi = block->sampleDistance(sample.position);
      ++result.stats.cache_query_count;
      if (compute_normals) {
        item.normal = normalizedOrZero(block->sampleGradient(sample.position));
        item.has_normal = item.normal.allFinite();
      }
    } else {
      if (!options.fallback_to_model_query) {
        result.warnings.push_back(
            "sample skipped because it does not fall in a resident active block.");
        continue;
      }
      source = "fallback";
      item.phi = model.sampleDistance(sample.position);
      ++result.stats.fallback_query_count;
      if (compute_normals) {
        item.normal = normalizedOrZero(model.sampleGradient(sample.position));
        item.has_normal = item.normal.allFinite();
      }
    }

    ++result.stats.queried_count;
    item.effective_phi = item.phi - item.radius;
    item.colliding = item.effective_phi <= options.threshold;
    item.within_threshold = item.colliding;
    if (!std::isfinite(item.phi) || !std::isfinite(item.effective_phi)) {
      result.error_message =
          "ActiveBlockQuery produced a non-finite distance for sample " +
          std::to_string(item.sample_id);
      return result;
    }

    min_initialized = true;
    min_phi = std::min(min_phi, item.phi);
    min_effective_phi = std::min(min_effective_phi, item.effective_phi);
    if (item.colliding) {
      result.colliding = true;
      ++result.stats.colliding_count;
    }

    if (options.include_non_colliding_samples || item.colliding) {
      result.samples.push_back(item);
      result.sample_sources.push_back(source);
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
  result.stats.result_count = result.samples.size();
  result.stats.cache_stats = cache.stats();
  result.stats.query_time_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  if (options.sort_results_by_effective_phi) {
    sortSamplesAndSources(result);
  }
  result.success = true;
  return result;
}

ActiveBlockQueryResult ActiveBlockQuery::queryWithSelection(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    ExpandedBlockCache& cache,
    const ActiveBlockSelectionOptions& selection_options,
    const ActiveBlockQueryOptions& query_options) {
  ActiveBlockQueryResult result;
  const ActiveBlockSelectionResult selection =
      ActiveBlockSelector::select(model, sample_set, selection_options);
  if (!selection.success) {
    result.error_message = selection.error_message;
    result.warnings = selection.warnings;
    return result;
  }
  result = query(
      model,
      sample_set,
      selection.block_ids,
      cache,
      query_options);
  result.warnings.insert(
      result.warnings.end(),
      selection.warnings.begin(),
      selection.warnings.end());
  return result;
}

}  // namespace adasdf
