#include "adasdf/cache/AdaptiveRefinementCache.h"

namespace adasdf {

AdaptiveRefinementCache::AdaptiveRefinementCache(
    const BuildCacheOptions& options)
    : options_(options),
      block_cache_(options.cache_max_entries),
      global_cache_(options.cache_max_entries) {
  stats_.sample_cache_enabled = options_.anySampleCacheEnabled();
  stats_.sample_cache_scope = options_.sample_cache;
}

void AdaptiveRefinementCache::setQuantization(
    const QuantizationOptions& quantization) {
  quantization_ = quantization;
}

bool AdaptiveRefinementCache::findSample(
    const Vector3& p,
    int level,
    CachedSDFSample* out) {
  if (options_.sample_cache == BuildCacheScope::Off) {
    return false;
  }
  const QuantizedPointKey key =
      QuantizedPointKeyBuilder::fromPoint(p, level, quantization_);
  return options_.sample_cache == BuildCacheScope::Global
             ? global_cache_.find(key, out)
             : block_cache_.find(key, out);
}

void AdaptiveRefinementCache::insertSample(
    const Vector3& p,
    int level,
    const CachedSDFSample& sample) {
  if (options_.sample_cache == BuildCacheScope::Off) {
    return;
  }
  const QuantizedPointKey key =
      QuantizedPointKeyBuilder::fromPoint(p, level, quantization_);
  if (options_.sample_cache == BuildCacheScope::Global) {
    global_cache_.insert(key, sample);
  } else {
    block_cache_.insert(key, sample);
  }
}

void AdaptiveRefinementCache::beginBlock(int block_id) {
  current_block_id_ = block_id;
  if (options_.sample_cache == BuildCacheScope::Block) {
    block_cache_.clear();
  }
}

void AdaptiveRefinementCache::endBlock(int block_id) {
  if (current_block_id_ == block_id) {
    current_block_id_ = -1;
  }
  stats_ = snapshotStats();
}

const BuildCacheStats& AdaptiveRefinementCache::stats() const {
  stats_ = snapshotStats();
  return stats_;
}

BuildCacheStats AdaptiveRefinementCache::snapshotStats() const {
  const SDFSampleCacheStats sample =
      options_.sample_cache == BuildCacheScope::Global
          ? global_cache_.snapshotStats()
          : block_cache_.snapshotStats();
  BuildCacheStats out;
  out.sample_cache_enabled = options_.sample_cache != BuildCacheScope::Off;
  out.sample_cache_scope = options_.sample_cache;
  out.sample_cache_entries = sample.entry_count;
  out.sample_cache_hits = sample.hit_count;
  out.sample_cache_misses = sample.miss_count;
  out.distance_cache_hits = options_.distance_cache ? sample.hit_count : 0;
  out.distance_cache_misses = options_.distance_cache ? sample.miss_count : 0;
  out.sign_cache_hits = options_.sign_cache ? sample.hit_count : 0;
  out.sign_cache_misses = options_.sign_cache ? sample.miss_count : 0;
  out.distance_queries_saved = sample.distance_query_saved;
  out.sign_queries_saved = sample.sign_query_saved;
  out.cache_memory_estimate_bytes =
      sample.entry_count * sizeof(CachedSDFSample);
  out.cache_lookup_time_ms = sample.lookup_time_ms;
  out.cache_insert_time_ms = sample.insert_time_ms;
  finalizeBuildCacheStats(&out);
  return out;
}

}  // namespace adasdf
