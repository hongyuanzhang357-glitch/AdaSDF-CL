#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <string>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/cache/SDFSampleCache.h"

namespace adasdf {

struct ExactSDFOracleOptions {
  int bvh_leaf_size = 8;
  int bvh_max_depth = 64;
  std::size_t max_cache_entries = 0;
  double cache_quantization_spacing = 0.0;
  bool require_signed_distance = true;
};

struct ExactSDFOracleStats {
  bool bvh_built = false;
  std::size_t bvh_node_count = 0;
  std::size_t bvh_leaf_count = 0;
  std::size_t bvh_triangle_count = 0;
  std::size_t exact_query_requests = 0;
  std::size_t unique_exact_queries = 0;
  std::size_t cache_hits = 0;
  std::size_t nearest_bvh_node_visits = 0;
  std::size_t sign_bvh_node_visits = 0;
  std::size_t nearest_triangle_tests = 0;
  std::size_t sign_triangle_tests = 0;
  std::size_t ambiguous_sign_count = 0;
  std::size_t brute_sign_fallback_count = 0;
  double bvh_build_time_ms = 0.0;
  double exact_query_time_ms = 0.0;

  std::size_t totalBVHNodeVisits() const {
    return nearest_bvh_node_visits + sign_bvh_node_visits;
  }

  std::size_t totalTriangleTests() const {
    return nearest_triangle_tests + sign_triangle_tests;
  }
};

struct ExactSDFOracleSample {
  bool success = false;
  bool cache_hit = false;
  double phi = 0.0;
  int nearest_triangle_id = -1;
  bool ambiguous_sign = false;
};

class ExactSDFOracle {
 public:
  bool reset(
      const TriangleMesh& mesh,
      const ExactSDFOracleOptions& options = ExactSDFOracleOptions(),
      std::string* error_message = nullptr);

  ExactSDFOracleSample sample(const Vector3& point);

  ExactSDFOracleSample sampleUnsignedDistance(
      const Vector3& point,
      double max_distance = std::numeric_limits<double>::infinity());

  bool valid() const { return sampler_.hasBVH(); }
  const ExactSDFOracleOptions& options() const { return options_; }
  const ExactSDFOracleStats& stats() const { return stats_; }
  const TriangleBVH& bvh() const { return sampler_.bvh(); }
  SDFSampleCacheStats cacheStats() const;

 private:
  ExactSDFOracleOptions options_;
  ExactSDFOracleStats stats_;
  BVHSDFSampler sampler_;
  QuantizationOptions quantization_;
  std::unique_ptr<SDFSampleCache> cache_;
  int unsigned_nearest_hint_ = -1;
};

}  // namespace adasdf
