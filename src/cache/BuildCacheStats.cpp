#include "adasdf/cache/BuildCacheStats.h"

namespace adasdf {

void finalizeBuildCacheStats(BuildCacheStats* stats) {
  if (stats == nullptr) {
    return;
  }
  const std::size_t lookups =
      stats->sample_cache_hits + stats->sample_cache_misses;
  stats->sample_cache_hit_rate =
      lookups > 0 ? static_cast<double>(stats->sample_cache_hits) /
                        static_cast<double>(lookups)
                  : 0.0;
}

void mergeBuildCacheStats(BuildCacheStats* dst, const BuildCacheStats& src) {
  if (dst == nullptr) {
    return;
  }
  dst->sample_cache_enabled =
      dst->sample_cache_enabled || src.sample_cache_enabled;
  if (src.sample_cache_scope == BuildCacheScope::Global ||
      dst->sample_cache_scope == BuildCacheScope::Off) {
    dst->sample_cache_scope = src.sample_cache_scope;
  }
  dst->sample_cache_entries += src.sample_cache_entries;
  dst->sample_cache_hits += src.sample_cache_hits;
  dst->sample_cache_misses += src.sample_cache_misses;
  dst->distance_cache_hits += src.distance_cache_hits;
  dst->distance_cache_misses += src.distance_cache_misses;
  dst->sign_cache_hits += src.sign_cache_hits;
  dst->sign_cache_misses += src.sign_cache_misses;
  dst->corner_cache_hits += src.corner_cache_hits;
  dst->corner_cache_misses += src.corner_cache_misses;
  dst->block_point_duplicate_count += src.block_point_duplicate_count;
  dst->marker_candidate_cache_hits += src.marker_candidate_cache_hits;
  dst->marker_candidate_cache_misses += src.marker_candidate_cache_misses;
  dst->marker_decision_cache_hits += src.marker_decision_cache_hits;
  dst->marker_decision_cache_misses += src.marker_decision_cache_misses;
  dst->distance_queries_saved += src.distance_queries_saved;
  dst->sign_queries_saved += src.sign_queries_saved;
  dst->box_triangle_distance_saved += src.box_triangle_distance_saved;
  dst->cache_memory_estimate_bytes += src.cache_memory_estimate_bytes;
  dst->cache_lookup_time_ms += src.cache_lookup_time_ms;
  dst->cache_insert_time_ms += src.cache_insert_time_ms;
  dst->deduplication_time_ms += src.deduplication_time_ms;
  dst->marker_cache_time_ms += src.marker_cache_time_ms;
  finalizeBuildCacheStats(dst);
}

}  // namespace adasdf
