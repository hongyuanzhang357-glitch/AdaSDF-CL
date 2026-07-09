#include "adasdf/cache/SDFSampleCache.h"

#include <chrono>

namespace adasdf {

namespace {

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

}  // namespace

SDFSampleCache::SDFSampleCache(std::size_t max_entries)
    : max_entries_(max_entries) {
  stats_.max_entry_count = max_entries;
}

bool SDFSampleCache::find(
    const QuantizedPointKey& key,
    CachedSDFSample* out) const {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  ++stats_.lookup_count;
  const auto it = samples_.find(key);
  if (it == samples_.end()) {
    ++stats_.miss_count;
    finalizeStatsUnlocked();
    stats_.lookup_time_ms +=
        elapsedMs(t0, std::chrono::steady_clock::now());
    return false;
  }
  if (out != nullptr) {
    *out = it->second;
  }
  ++stats_.hit_count;
  ++stats_.distance_query_saved;
  if (it->second.sign_known) {
    ++stats_.sign_query_saved;
  }
  finalizeStatsUnlocked();
  stats_.lookup_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
  return true;
}

void SDFSampleCache::insert(
    const QuantizedPointKey& key,
    const CachedSDFSample& sample) {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  if (max_entries_ > 0 && samples_.size() >= max_entries_ &&
      samples_.find(key) == samples_.end()) {
    finalizeStatsUnlocked();
    stats_.insert_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
    return;
  }
  const auto inserted = samples_.emplace(key, sample);
  if (!inserted.second) {
    inserted.first->second = sample;
  } else {
    ++stats_.insert_count;
  }
  finalizeStatsUnlocked();
  stats_.insert_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
}

void SDFSampleCache::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  samples_.clear();
  stats_ = {};
  stats_.max_entry_count = max_entries_;
}

const SDFSampleCacheStats& SDFSampleCache::stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  finalizeStatsUnlocked();
  return stats_;
}

SDFSampleCacheStats SDFSampleCache::snapshotStats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  finalizeStatsUnlocked();
  return stats_;
}

std::size_t SDFSampleCache::entryCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return samples_.size();
}

void SDFSampleCache::finalizeStatsUnlocked() const {
  stats_.entry_count = samples_.size();
  stats_.hit_rate =
      stats_.lookup_count > 0
          ? static_cast<double>(stats_.hit_count) /
                static_cast<double>(stats_.lookup_count)
          : 0.0;
}

}  // namespace adasdf
