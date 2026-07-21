#include "adasdf/acceleration/ExactSDFOracle.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace adasdf {

bool ExactSDFOracle::reset(
    const TriangleMesh& mesh,
    const ExactSDFOracleOptions& options,
    std::string* error_message) {
  options_ = options;
  stats_ = {};
  unsigned_nearest_hint_ = -1;
  cache_ = std::make_unique<SDFSampleCache>(options.max_cache_entries);
  if (mesh.empty()) {
    if (error_message != nullptr) {
      *error_message = "ExactSDFOracle requires a non-empty triangle mesh";
    }
    return false;
  }
  if (options.bvh_leaf_size <= 0 || options.bvh_max_depth <= 0) {
    if (error_message != nullptr) {
      *error_message = "ExactSDFOracle BVH limits must be positive";
    }
    return false;
  }

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = options.require_signed_distance;
  sampler_options.fallback_to_bruteforce_sign = true;
  sampler_options.bvh_options.max_leaf_size = options.bvh_leaf_size;
  sampler_options.bvh_options.max_depth = options.bvh_max_depth;
  BuildAccelerationStats build_stats;
  if (!sampler_.reset(mesh, sampler_options, &build_stats)) {
    if (error_message != nullptr) {
      *error_message = "ExactSDFOracle failed to build triangle BVH";
    }
    return false;
  }

  const MeshAABB bounds = mesh.aabb();
  quantization_.origin = toVector3(bounds.min);
  quantization_.spacing = options.cache_quantization_spacing > 0.0
      ? options.cache_quantization_spacing
      : std::max(mesh.diagonalLength() * 1.0e-12, 1.0e-15);
  quantization_.epsilon = std::max(quantization_.spacing * 0.25, 1.0e-16);
  quantization_.include_level = false;

  stats_.bvh_built = true;
  stats_.bvh_node_count = build_stats.bvh_node_count;
  stats_.bvh_leaf_count = build_stats.bvh_leaf_count;
  stats_.bvh_triangle_count = build_stats.bvh_triangle_count;
  stats_.bvh_build_time_ms = build_stats.bvh_build_time_ms;
  if (error_message != nullptr) {
    error_message->clear();
  }
  return true;
}

ExactSDFOracleSample ExactSDFOracle::sample(const Vector3& point) {
  const auto begin = std::chrono::steady_clock::now();
  struct QueryTimer {
    std::chrono::steady_clock::time_point begin;
    double* elapsed_ms = nullptr;
    ~QueryTimer() {
      *elapsed_ms += std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - begin).count();
    }
  } timer{begin, &stats_.exact_query_time_ms};
  ExactSDFOracleSample out;
  ++stats_.exact_query_requests;
  if (!valid() || !point.allFinite() || cache_ == nullptr) {
    return out;
  }

  const QuantizedPointKey key =
      QuantizedPointKeyBuilder::fromPoint(point, 0, quantization_);
  CachedSDFSample cached;
  if (cache_->find(key, &cached)) {
    if (!options_.require_signed_distance || cached.sign_known) {
      ++stats_.cache_hits;
      out.success = cached.finite;
      out.cache_hit = true;
      out.phi = cached.phi;
      out.nearest_triangle_id = cached.nearest_triangle_id;
      return out;
    }
  }

  const BVHSDFSampleResult sampled = sampler_.sample(point);
  ++stats_.unique_exact_queries;
  stats_.nearest_bvh_node_visits += sampled.nearest.node_visits;
  stats_.sign_bvh_node_visits += sampled.ray.node_visits;
  stats_.nearest_triangle_tests += sampled.nearest.triangle_tests;
  stats_.sign_triangle_tests += sampled.ray.triangle_tests;
  stats_.ambiguous_sign_count += sampled.ambiguous_sign ? 1u : 0u;
  stats_.brute_sign_fallback_count += sampled.fallback_sign ? 1u : 0u;

  out.success = sampled.success && std::isfinite(sampled.phi);
  out.phi = out.success ? sampled.phi : 0.0;
  out.nearest_triangle_id = sampled.nearest.triangle_index;
  out.ambiguous_sign = sampled.ambiguous_sign;
  if (out.success) {
    CachedSDFSample value;
    value.phi = out.phi;
    value.sign_known = !out.ambiguous_sign;
    value.sign = out.phi < 0.0 ? -1 : (out.phi > 0.0 ? 1 : 0);
    value.nearest_triangle_id = out.nearest_triangle_id;
    value.finite = true;
    cache_->insert(key, value);
  }
  return out;
}

ExactSDFOracleSample ExactSDFOracle::sampleUnsignedDistance(
    const Vector3& point,
    double max_distance) {
  const auto begin = std::chrono::steady_clock::now();
  struct QueryTimer {
    std::chrono::steady_clock::time_point begin;
    double* elapsed_ms = nullptr;
    ~QueryTimer() {
      *elapsed_ms += std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - begin).count();
    }
  } timer{begin, &stats_.exact_query_time_ms};
  ExactSDFOracleSample out;
  ++stats_.exact_query_requests;
  if (!valid() || !point.allFinite() || cache_ == nullptr) {
    return out;
  }

  const QuantizedPointKey key =
      QuantizedPointKeyBuilder::fromPoint(point, 0, quantization_);
  CachedSDFSample cached;
  if (cache_->find(key, &cached)) {
    ++stats_.cache_hits;
    const double distance = std::abs(cached.phi);
    const bool within_bound = !std::isfinite(max_distance) ||
        max_distance < 0.0 || distance <= max_distance + 1.0e-15;
    out.success = cached.finite && within_bound;
    out.cache_hit = true;
    out.phi = distance;
    out.nearest_triangle_id = cached.nearest_triangle_id;
    return out;
  }

  BVHNearestTriangleQueryOptions nearest_options;
  nearest_options.max_distance = max_distance;
  nearest_options.initial_triangle_index = unsigned_nearest_hint_;
  const BVHNearestTriangleQueryResult nearest =
      BVHNearestTriangleQuery::query(sampler_.bvh(), point, nearest_options);
  ++stats_.unique_exact_queries;
  stats_.nearest_bvh_node_visits += nearest.node_visits;
  stats_.nearest_triangle_tests += nearest.triangle_tests;
  out.success = nearest.success && std::isfinite(nearest.distance);
  out.phi = out.success ? nearest.distance : 0.0;
  out.nearest_triangle_id = nearest.triangle_index;
  if (out.success) {
    unsigned_nearest_hint_ = out.nearest_triangle_id;
    CachedSDFSample value;
    value.phi = out.phi;
    value.sign_known = false;
    value.sign = 0;
    value.nearest_triangle_id = out.nearest_triangle_id;
    value.finite = true;
    cache_->insert(key, value);
  }
  return out;
}

SDFSampleCacheStats ExactSDFOracle::cacheStats() const {
  return cache_ != nullptr ? cache_->snapshotStats() : SDFSampleCacheStats{};
}

}  // namespace adasdf
