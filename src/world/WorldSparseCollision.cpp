#include "adasdf/world/WorldSparseCollision.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

CollisionSampleSet transformSamplesForTarget(
    const WorldObject& source,
    const WorldObject& target) {
  CollisionSampleSet transformed;
  for (const CollisionSample& sample : source.samples.samples) {
    CollisionSample item = sample;
    const Vector3 world_point = source.transform.applyPoint(sample.position);
    item.position = target.transform.inverseApplyPoint(world_point);
    item.object_id = source.object_id;
    item.group_id = target.object_id;
    transformed.add(item);
  }
  return transformed;
}

void appendQueryResults(
    const WorldObject& source,
    const WorldObject& target,
    int pair_id,
    const SparseCollisionResult& query,
    WorldSparsePairResult* pair_result) {
  pair_result->sample_count += query.sample_count;
  pair_result->queried_count += query.queried_count;
  pair_result->colliding = pair_result->colliding || query.colliding;
  pair_result->early_exit_triggered =
      pair_result->early_exit_triggered || query.early_exit_triggered;

  if (query.queried_count > 0) {
    if (pair_result->queried_count == query.queried_count) {
      pair_result->min_phi = query.min_phi;
      pair_result->min_effective_phi = query.min_effective_phi;
    } else {
      pair_result->min_phi = std::min(pair_result->min_phi, query.min_phi);
      pair_result->min_effective_phi =
          std::min(pair_result->min_effective_phi, query.min_effective_phi);
    }
  }

  for (SparseSDFSampleResult sample : query.violations) {
    WorldSparseSampleResult item;
    item.pair_id = pair_id;
    item.source_object_id = source.object_id;
    item.target_object_id = target.object_id;
    item.source_name = worldObjectDisplayName(source);
    item.target_name = worldObjectDisplayName(target);
    item.world_position = source.transform.applyPoint(
        source.samples.samples.empty() ? Vector3{} : sample.position);
    item.sample = std::move(sample);
    item.world_position = target.transform.applyPoint(item.sample.position);
    if (item.sample.has_normal) {
      item.world_normal = target.transform.applyVector(item.sample.normal);
      item.has_world_normal = item.world_normal.allFinite();
    }
    pair_result->violations.push_back(std::move(item));
  }
  pair_result->violation_count = pair_result->violations.size();
}

SparseCollisionQueryOptions sparseOptionsFromWorld(
    const WorldSparseCollisionOptions& options) {
  SparseCollisionQueryOptions sparse_options;
  sparse_options.mode = options.mode;
  sparse_options.threshold = options.threshold;
  sparse_options.early_exit = options.early_exit;
  sparse_options.compute_normals = options.compute_normals;
  sparse_options.use_sample_radius = options.use_sample_radius;
  sparse_options.return_all_violations = options.return_all_violations;
  return sparse_options;
}

void updateGlobalStats(
    const WorldSparsePairResult& pair,
    WorldSparseCollisionResult* result) {
  result->colliding = result->colliding || pair.colliding;
  result->stats.sample_count += pair.sample_count;
  result->stats.queried_sample_count += pair.queried_count;
  result->stats.violation_count += pair.violation_count;

  if (pair.queried_count == 0) {
    return;
  }
  if (result->stats.queried_sample_count == pair.queried_count) {
    result->stats.min_phi = pair.min_phi;
    result->stats.min_effective_phi = pair.min_effective_phi;
  } else {
    result->stats.min_phi = std::min(result->stats.min_phi, pair.min_phi);
    result->stats.min_effective_phi =
        std::min(result->stats.min_effective_phi, pair.min_effective_phi);
  }
}

}  // namespace

WorldSparseCollisionResult WorldSparseCollision::check(
    const CollisionWorld& world,
    const WorldSparseCollisionOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  WorldSparseCollisionResult result;
  result.broadphase = AABBBroadphase::compute(world, options.broadphase_options);
  if (!result.broadphase.success) {
    result.error_message = result.broadphase.error_message;
    return result;
  }
  result.stats.broadphase_pair_count = result.broadphase.pairs.size();

  const SparseCollisionQueryOptions sparse_options =
      sparseOptionsFromWorld(options);
  for (const WorldPair& pair : result.broadphase.pairs) {
    const WorldObject* a = world.findObject(pair.object_a);
    const WorldObject* b = world.findObject(pair.object_b);
    if (!a || !b) {
      result.error_message = "broadphase pair references a missing world object";
      return result;
    }
    WorldSparsePairResult pair_result;
    pair_result.pair_id = pair.pair_id;
    pair_result.object_a = a->object_id;
    pair_result.object_b = b->object_id;
    pair_result.name_a = worldObjectDisplayName(*a);
    pair_result.name_b = worldObjectDisplayName(*b);

    if (!hasQueryableModel(*a) || !hasQueryableModel(*b)) {
      result.warnings.push_back(
          "pair " + std::to_string(pair.pair_id) +
          " skipped because one object has no queryable SDF model");
      result.pairs.push_back(pair_result);
      continue;
    }

    bool queried_any = false;
    if (a->has_samples && !a->samples.empty()) {
      const CollisionSampleSet samples = transformSamplesForTarget(*a, *b);
      const SparseCollisionResult query =
          SparseCollisionQuery::check(*b->model, samples, sparse_options);
      if (!query.success) {
        result.error_message = query.error_message;
        return result;
      }
      appendQueryResults(*a, *b, pair.pair_id, query, &pair_result);
      queried_any = true;
    }

    const bool stop_pair =
        options.mode == SparseCollisionMode::CollisionOnly &&
        options.early_exit && pair_result.colliding;
    if (options.bidirectional && !stop_pair && b->has_samples &&
        !b->samples.empty()) {
      const CollisionSampleSet samples = transformSamplesForTarget(*b, *a);
      const SparseCollisionResult query =
          SparseCollisionQuery::check(*a->model, samples, sparse_options);
      if (!query.success) {
        result.error_message = query.error_message;
        return result;
      }
      appendQueryResults(*b, *a, pair.pair_id, query, &pair_result);
      queried_any = true;
    }

    if (!queried_any) {
      result.warnings.push_back(
          "pair " + std::to_string(pair.pair_id) +
          " skipped because neither object has collision samples");
    } else {
      ++result.stats.queried_pair_count;
    }

    std::sort(
        pair_result.violations.begin(),
        pair_result.violations.end(),
        [](const WorldSparseSampleResult& x, const WorldSparseSampleResult& y) {
          if (x.sample.effective_phi != y.sample.effective_phi) {
            return x.sample.effective_phi < y.sample.effective_phi;
          }
          if (x.source_object_id != y.source_object_id) {
            return x.source_object_id < y.source_object_id;
          }
          return x.sample.sample_id < y.sample.sample_id;
        });
    pair_result.violation_count = pair_result.violations.size();
    updateGlobalStats(pair_result, &result);
    result.pairs.push_back(std::move(pair_result));

    if (options.mode == SparseCollisionMode::CollisionOnly &&
        options.early_exit && result.colliding) {
      break;
    }
  }

  result.stats.elapsed_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  result.success = true;
  return result;
}

const char* toString(WorldSparseCollisionOptions options) {
  return toString(options.mode);
}

}  // namespace adasdf
