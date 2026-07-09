#include "adasdf/sampling/ContactBandBlockSampler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "adasdf/cache/BlockPointDeduplicator.h"
#include "adasdf/cache/SDFSampleCache.h"

namespace adasdf {
namespace {

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

Vector3 gridPoint(const AABB& bounds, int i, int j, int k, int n) {
  const double tx = n <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
  const double ty = n <= 1 ? 0.0 : static_cast<double>(j) / static_cast<double>(n - 1);
  const double tz = n <= 1 ? 0.0 : static_cast<double>(k) / static_cast<double>(n - 1);
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * tx,
      bounds.min.y + (bounds.max.y - bounds.min.y) * ty,
      bounds.min.z + (bounds.max.z - bounds.min.z) * tz};
}

AdaptiveSDFBlock makeBlockHeader(
    const AABB& bounds,
    int block_id,
    int octree_node_id,
    int level,
    int resolution,
    bool signed_distance) {
  AdaptiveSDFBlock block;
  block.block_id = block_id;
  block.octree_node_id = octree_node_id;
  block.level = level;
  block.bounds = bounds;
  block.nx = resolution;
  block.ny = resolution;
  block.nz = resolution;
  block.origin = bounds.min;
  block.spacing = {
      resolution <= 1 ? 0.0 : (bounds.max.x - bounds.min.x) /
                                 static_cast<double>(resolution - 1),
      resolution <= 1 ? 0.0 : (bounds.max.y - bounds.min.y) /
                                 static_cast<double>(resolution - 1),
      resolution <= 1 ? 0.0 : (bounds.max.z - bounds.min.z) /
                                 static_cast<double>(resolution - 1)};
  block.signed_distance = signed_distance;
  return block;
}

BVHSDFSampleResult sampleExactPoint(
    BVHSDFSampler& sampler,
    const Vector3& point,
    bool signed_distance) {
  const TriangleMesh* mesh = sampler.mesh();
  if (mesh != nullptr &&
      sampler.options().acceleration == SDFSamplingAcceleration::BVH &&
      sampler.hasBVH() &&
      !sampler.options().enable_counters) {
    return BVHSDFSampler::sampleWithBVH(
        *mesh,
        sampler.bvh(),
        point,
        sampler.options());
  }
  if (mesh != nullptr && !sampler.options().enable_counters) {
    return BVHSDFSampler::sampleBruteForce(*mesh, point, signed_distance);
  }
  return sampler.sample(point);
}

CachedSDFSample toCachedSample(
    const BVHSDFSampleResult& sample,
    bool signed_distance) {
  CachedSDFSample cached;
  cached.phi = sample.success ? sample.phi : 0.0;
  cached.sign_known = signed_distance && sample.success;
  cached.sign = cached.phi < 0.0 ? -1 : (cached.phi > 0.0 ? 1 : 0);
  cached.nearest_triangle_id = sample.nearest.triangle_index;
  cached.finite = std::isfinite(cached.phi);
  return cached;
}

double blockQuantizationSpacing(const AABB& bounds, int n, int coarse_n) {
  const int exact_den = std::max(1, n - 1);
  const int coarse_den = std::max(1, coarse_n - 1);
  const int common_den = std::max(1, std::lcm(exact_den, coarse_den));
  double spacing = std::numeric_limits<double>::infinity();
  const auto consider = [&](double value) {
    if (value > 0.0) {
      spacing = std::min(spacing, value);
    }
  };
  consider((bounds.max.x - bounds.min.x) / static_cast<double>(common_den));
  consider((bounds.max.y - bounds.min.y) / static_cast<double>(common_den));
  consider((bounds.max.z - bounds.min.z) / static_cast<double>(common_den));
  return std::isfinite(spacing) ? spacing : 1.0;
}

BVHSDFSampleResult sampleExactPointCached(
    BVHSDFSampler& sampler,
    const Vector3& point,
    bool signed_distance,
    int level,
    const QuantizationOptions& quantization,
    const BuildCacheOptions& cache_options,
    SDFSampleCache* sample_cache) {
  if (sample_cache == nullptr ||
      cache_options.sample_cache == BuildCacheScope::Off) {
    return sampleExactPoint(sampler, point, signed_distance);
  }
  const QuantizedPointKey key =
      QuantizedPointKeyBuilder::fromPoint(point, level, quantization);
  CachedSDFSample cached;
  if (sample_cache->find(key, &cached)) {
    BVHSDFSampleResult result;
    result.success = cached.finite;
    result.phi = cached.phi;
    result.nearest.triangle_index = cached.nearest_triangle_id;
    return result;
  }
  BVHSDFSampleResult result = sampleExactPoint(sampler, point, signed_distance);
  sample_cache->insert(key, toCachedSample(result, signed_distance));
  return result;
}

BuildCacheStats buildCacheStatsFromSampleCache(
    const BuildCacheOptions& options,
    const SDFSampleCache& cache,
    std::size_t duplicate_count,
    const ContactBandMarkerCacheStats& marker_stats) {
  const SDFSampleCacheStats sample = cache.snapshotStats();
  BuildCacheStats stats;
  stats.sample_cache_enabled = options.sample_cache != BuildCacheScope::Off;
  stats.sample_cache_scope = options.sample_cache;
  stats.sample_cache_entries = sample.entry_count;
  stats.sample_cache_hits = sample.hit_count;
  stats.sample_cache_misses = sample.miss_count;
  stats.distance_cache_hits = options.distance_cache ? sample.hit_count : 0;
  stats.distance_cache_misses = options.distance_cache ? sample.miss_count : 0;
  stats.sign_cache_hits = options.sign_cache ? sample.hit_count : 0;
  stats.sign_cache_misses = options.sign_cache ? sample.miss_count : 0;
  stats.corner_cache_hits = options.corner_cache != BuildCacheScope::Off
                                 ? sample.hit_count
                                 : 0;
  stats.corner_cache_misses = options.corner_cache != BuildCacheScope::Off
                                   ? sample.miss_count
                                   : 0;
  stats.block_point_duplicate_count = duplicate_count;
  stats.marker_candidate_cache_hits = marker_stats.candidate_hit_count;
  stats.marker_candidate_cache_misses = marker_stats.candidate_miss_count;
  stats.marker_decision_cache_hits = marker_stats.decision_hit_count;
  stats.marker_decision_cache_misses = marker_stats.decision_miss_count;
  stats.distance_queries_saved = sample.distance_query_saved;
  stats.sign_queries_saved = sample.sign_query_saved;
  stats.box_triangle_distance_saved = marker_stats.box_triangle_distance_saved;
  stats.cache_memory_estimate_bytes =
      sample.entry_count * sizeof(CachedSDFSample);
  stats.cache_lookup_time_ms = sample.lookup_time_ms;
  stats.cache_insert_time_ms = sample.insert_time_ms;
  stats.marker_cache_time_ms = marker_stats.marker_cache_time_ms;
  finalizeBuildCacheStats(&stats);
  return stats;
}

double trilerp(
    const std::vector<double>& values,
    int n,
    double x,
    double y,
    double z) {
  const auto clamp01 = [](double value) {
    return std::max(0.0, std::min(1.0, value));
  };
  x = clamp01(x);
  y = clamp01(y);
  z = clamp01(z);
  const double gx = x * static_cast<double>(n - 1);
  const double gy = y * static_cast<double>(n - 1);
  const double gz = z * static_cast<double>(n - 1);
  const int x0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gx))));
  const int y0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gy))));
  const int z0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gz))));
  const int x1 = std::min(n - 1, x0 + 1);
  const int y1 = std::min(n - 1, y0 + 1);
  const int z1 = std::min(n - 1, z0 + 1);
  const double tx = gx - static_cast<double>(x0);
  const double ty = gy - static_cast<double>(y0);
  const double tz = gz - static_cast<double>(z0);
  const auto at = [&](int i, int j, int k) {
    return values[gridIndex(i, j, k, n, n)];
  };
  const double c00 = at(x0, y0, z0) * (1.0 - tx) + at(x1, y0, z0) * tx;
  const double c10 = at(x0, y1, z0) * (1.0 - tx) + at(x1, y1, z0) * tx;
  const double c01 = at(x0, y0, z1) * (1.0 - tx) + at(x1, y0, z1) * tx;
  const double c11 = at(x0, y1, z1) * (1.0 - tx) + at(x1, y1, z1) * tx;
  const double c0 = c00 * (1.0 - ty) + c10 * ty;
  const double c1 = c01 * (1.0 - ty) + c11 * ty;
  return c0 * (1.0 - tz) + c1 * tz;
}

double applyFarFieldMode(
    double value,
    double reference_sign_value,
    const ContactBandSamplingOptions& options) {
  if (options.far_field_mode == ContactBandFarFieldMode::CoarseInterpolate) {
    return value;
  }
  const double sign = reference_sign_value < 0.0 ? -1.0 : 1.0;
  if (options.far_field_mode == ContactBandFarFieldMode::ConstantSign) {
    return sign * std::abs(value);
  }
  const double width = std::max(options.contact_band_width, 1e-12);
  return sign * std::max(width, std::abs(value));
}

bool shouldExactNode(
    const ContactBandMask& mask,
    std::size_t index,
    bool has_contact_band,
    const ContactBandSamplingOptions& options) {
  if (!has_contact_band || index >= mask.exact_required.size() ||
      mask.exact_required[index] == 0) {
    return false;
  }
  if (mask.contact_band_node[index] != 0) {
    return options.exact_contact_band_nodes;
  }
  if (mask.halo_node[index] != 0) {
    return options.exact_halo_nodes;
  }
  return options.exact_contact_band_nodes;
}

}  // namespace

ContactBandBlockSamplingResult ContactBandBlockSampler::sampleBlock(
    const AABB& block_bounds,
    int block_id,
    int octree_node_id,
    int level,
    int block_resolution,
    bool signed_distance,
      BVHSDFSampler& exact_sampler,
      const TriangleBVH& triangle_bvh,
      const ContactBandSamplingOptions& options,
      const BuildCacheOptions* cache_options,
      ContactBandMarkerCache* marker_cache) {
  const auto total0 = std::chrono::steady_clock::now();
  ContactBandBlockSamplingResult result;
  const int n = std::max(2, block_resolution);
  BuildCacheOptions effective_cache;
  if (cache_options != nullptr) {
    effective_cache = *cache_options;
  } else {
    effective_cache.sample_cache = BuildCacheScope::Off;
    effective_cache.corner_cache = BuildCacheScope::Off;
    effective_cache.sign_cache = false;
    effective_cache.distance_cache = false;
    effective_cache.marker_cache = false;
  }
  if (!effective_cache.marker_cache) {
    marker_cache = nullptr;
  }
  SDFSampleCache sample_cache(effective_cache.cache_max_entries);
  result.block = makeBlockHeader(
      block_bounds,
      block_id,
      octree_node_id,
      level,
      n,
      signed_distance);
  result.block.phi.assign(
      static_cast<std::size_t>(n) * static_cast<std::size_t>(n) *
          static_cast<std::size_t>(n),
      0.0);
  result.mask = ContactBandMarker::markBlock(
      block_bounds,
      n,
      triangle_bvh,
      options.markerOptions(),
      marker_cache,
      block_id,
      level);
  result.has_contact_band = result.mask.contact_band_node_count > 0;

  const int coarse_n = std::max(2, options.far_field_resolution);
  QuantizationOptions quantization;
  quantization.origin = block_bounds.min;
  quantization.spacing = blockQuantizationSpacing(block_bounds, n, coarse_n);
  quantization.epsilon = effective_cache.cache_quantization_epsilon;
  quantization.include_level = true;
  std::vector<double> coarse(
      static_cast<std::size_t>(coarse_n) * static_cast<std::size_t>(coarse_n) *
          static_cast<std::size_t>(coarse_n),
      0.0);
  const auto coarse0 = std::chrono::steady_clock::now();
  for (int k = 0; k < coarse_n; ++k) {
    for (int j = 0; j < coarse_n; ++j) {
      for (int i = 0; i < coarse_n; ++i) {
        const BVHSDFSampleResult sample =
            sampleExactPointCached(
                exact_sampler,
                gridPoint(block_bounds, i, j, k, coarse_n),
                signed_distance,
                level,
                quantization,
                effective_cache,
                &sample_cache);
        coarse[gridIndex(i, j, k, coarse_n, coarse_n)] =
            sample.success ? sample.phi : 0.0;
        ++result.coarse_sample_count;
      }
    }
  }
  const auto coarse1 = std::chrono::steady_clock::now();
  result.coarse_sampling_time_ms = elapsedMs(coarse0, coarse1);

  const double reference_sign_value = coarse.empty() ? 1.0 : coarse[coarse.size() / 2];
  const auto interp0 = std::chrono::steady_clock::now();
  for (int k = 0; k < n; ++k) {
    const double z = n <= 1 ? 0.0 : static_cast<double>(k) / static_cast<double>(n - 1);
    for (int j = 0; j < n; ++j) {
      const double y = n <= 1 ? 0.0 : static_cast<double>(j) / static_cast<double>(n - 1);
      for (int i = 0; i < n; ++i) {
        const double x = n <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
        const std::size_t index = gridIndex(i, j, k, n, n);
        result.block.phi[index] =
            applyFarFieldMode(
                trilerp(coarse, coarse_n, x, y, z),
                reference_sign_value,
                options);
      }
    }
  }
  const auto interp1 = std::chrono::steady_clock::now();
  result.interpolation_time_ms = elapsedMs(interp0, interp1);

  const auto exact0 = std::chrono::steady_clock::now();
  std::vector<std::size_t> exact_indices;
  std::vector<Vector3> exact_points;
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        const std::size_t index = gridIndex(i, j, k, n, n);
        if (!shouldExactNode(result.mask, index, result.has_contact_band, options)) {
          continue;
        }
        exact_indices.push_back(index);
        exact_points.push_back(gridPoint(block_bounds, i, j, k, n));
      }
    }
  }
  const auto dedup0 = std::chrono::steady_clock::now();
  const DeduplicatedPointSet dedup = BlockPointDeduplicator::deduplicate(
      exact_points,
      level,
      quantization);
  const auto dedup1 = std::chrono::steady_clock::now();
  std::vector<double> unique_phi(dedup.unique_points.size(), 0.0);
  for (std::size_t i = 0; i < dedup.unique_points.size(); ++i) {
    const BVHSDFSampleResult sample =
        sampleExactPointCached(
            exact_sampler,
            dedup.unique_points[i],
            signed_distance,
            level,
            quantization,
            effective_cache,
            &sample_cache);
    unique_phi[i] = sample.success ? sample.phi : 0.0;
  }
  for (std::size_t i = 0; i < exact_indices.size(); ++i) {
    const int unique_index = i < dedup.reverse_index.size()
                                 ? dedup.reverse_index[i]
                                 : -1;
    if (unique_index >= 0 &&
        static_cast<std::size_t>(unique_index) < unique_phi.size()) {
      result.block.phi[exact_indices[i]] =
          unique_phi[static_cast<std::size_t>(unique_index)];
    }
  }
  result.exact_node_count = exact_indices.size();
  const auto exact1 = std::chrono::steady_clock::now();
  result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
  result.predicted_node_count = result.block.phi.size() - result.exact_node_count;
  result.far_field_node_count =
      result.has_contact_band ? result.predicted_node_count : result.block.phi.size();

  const auto total1 = std::chrono::steady_clock::now();
  result.total_time_ms = elapsedMs(total0, total1);
  result.success = true;
  result.diagnostics.marker_mode = result.mask.marker_mode;
  result.diagnostics.total_block_count = 1;
  result.diagnostics.contact_band_block_count = result.has_contact_band ? 1 : 0;
  result.diagnostics.far_field_block_count = result.has_contact_band ? 0 : 1;
  result.diagnostics.total_node_count = result.block.phi.size();
  result.diagnostics.exact_node_count = result.exact_node_count;
  result.diagnostics.predicted_node_count = result.predicted_node_count;
  result.diagnostics.far_field_node_count = result.far_field_node_count;
  result.diagnostics.coarse_sample_count = result.coarse_sample_count;
  result.diagnostics.distance_query_count =
      effective_cache.sample_cache == BuildCacheScope::Off
          ? result.exact_node_count + result.coarse_sample_count
          : sample_cache.snapshotStats().miss_count;
  result.diagnostics.sign_query_count =
      signed_distance ? result.diagnostics.distance_query_count : 0;
  result.diagnostics.candidate_triangle_aabb_overlap_count =
      result.mask.candidate_triangle_aabb_overlap_count;
  result.diagnostics.candidate_cell_count =
      result.mask.candidate_cell_count;
  result.diagnostics.candidate_triangle_count =
      result.mask.candidate_triangle_count;
  result.diagnostics.distance_refined_cell_count =
      result.mask.distance_refined_cell_count;
  result.diagnostics.refined_candidate_count =
      result.mask.refined_candidate_count;
  result.diagnostics.distance_rejected_cell_count =
      result.mask.distance_rejected_cell_count;
  result.diagnostics.rejected_candidate_count =
      result.mask.rejected_candidate_count;
  result.diagnostics.marked_cell_count = result.mask.marked_cell_count;
  result.diagnostics.accepted_contact_cell_count =
      result.mask.accepted_contact_cell_count;
  result.diagnostics.marked_node_count = result.mask.marked_node_count;
  result.diagnostics.local_halo_node_count =
      result.mask.local_halo_node_count;
  result.diagnostics.global_halo_node_count =
      result.mask.global_halo_node_count;
  result.diagnostics.overmark_ratio_estimate =
      result.mask.overmark_ratio_estimate;
  result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
  result.diagnostics.coarse_sampling_time_ms = result.coarse_sampling_time_ms;
  result.diagnostics.interpolation_time_ms = result.interpolation_time_ms;
  result.diagnostics.total_time_ms = result.total_time_ms;
  result.diagnostics.marker_time_ms = result.mask.marker_time_ms;
  result.diagnostics.distance_refinement_time_ms =
      result.mask.distance_refinement_time_ms;
  result.diagnostics.marker_refinement_time_ms =
      result.mask.marker_refinement_time_ms;
  result.diagnostics.box_triangle_distance_time_ms =
      result.mask.box_triangle_distance_time_ms;
  result.diagnostics.triangle_bvh_query_time_ms =
      result.mask.triangle_bvh_query_time_ms;
  finalizeContactBandDiagnostics(&result.diagnostics);
  const ContactBandMarkerCacheStats marker_stats =
      marker_cache != nullptr ? marker_cache->snapshotStats()
                              : ContactBandMarkerCacheStats{};
  result.cache_stats = buildCacheStatsFromSampleCache(
      effective_cache,
      sample_cache,
      dedup.duplicate_count,
      marker_stats);
  result.cache_stats.deduplication_time_ms =
      elapsedMs(dedup0, dedup1);
  return result;
}

}  // namespace adasdf
