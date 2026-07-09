#pragma once

#include <cstddef>

#include "adasdf/cache/BuildCacheOptions.h"

namespace adasdf {

struct BuildCacheStats {
  bool sample_cache_enabled = false;
  BuildCacheScope sample_cache_scope = BuildCacheScope::Off;
  std::size_t sample_cache_entries = 0;
  std::size_t sample_cache_hits = 0;
  std::size_t sample_cache_misses = 0;
  double sample_cache_hit_rate = 0.0;

  std::size_t distance_cache_hits = 0;
  std::size_t distance_cache_misses = 0;
  std::size_t sign_cache_hits = 0;
  std::size_t sign_cache_misses = 0;
  std::size_t corner_cache_hits = 0;
  std::size_t corner_cache_misses = 0;
  std::size_t block_point_duplicate_count = 0;
  std::size_t marker_candidate_cache_hits = 0;
  std::size_t marker_candidate_cache_misses = 0;
  std::size_t marker_decision_cache_hits = 0;
  std::size_t marker_decision_cache_misses = 0;
  std::size_t distance_queries_saved = 0;
  std::size_t sign_queries_saved = 0;
  std::size_t box_triangle_distance_saved = 0;
  std::size_t cache_memory_estimate_bytes = 0;

  double cache_lookup_time_ms = 0.0;
  double cache_insert_time_ms = 0.0;
  double deduplication_time_ms = 0.0;
  double marker_cache_time_ms = 0.0;
};

void finalizeBuildCacheStats(BuildCacheStats* stats);
void mergeBuildCacheStats(BuildCacheStats* dst, const BuildCacheStats& src);

}  // namespace adasdf
