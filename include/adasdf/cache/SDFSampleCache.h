#pragma once

#include <cstddef>
#include <mutex>
#include <unordered_map>

#include "adasdf/cache/QuantizedPointKey.h"
#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct CachedSDFSample {
  double phi = 0.0;
  bool sign_known = false;
  int sign = 0;
  int nearest_triangle_id = -1;
  Vector3 nearest_point;
  bool finite = true;
};

struct SDFSampleCacheStats {
  std::size_t lookup_count = 0;
  std::size_t hit_count = 0;
  std::size_t miss_count = 0;
  std::size_t insert_count = 0;
  double hit_rate = 0.0;
  std::size_t distance_query_saved = 0;
  std::size_t sign_query_saved = 0;
  std::size_t entry_count = 0;
  std::size_t max_entry_count = 0;
  double lookup_time_ms = 0.0;
  double insert_time_ms = 0.0;
};

class SDFSampleCache {
 public:
  explicit SDFSampleCache(std::size_t max_entries = 0);

  bool find(const QuantizedPointKey& key, CachedSDFSample* out) const;
  void insert(const QuantizedPointKey& key, const CachedSDFSample& sample);
  void clear();
  const SDFSampleCacheStats& stats() const;
  SDFSampleCacheStats snapshotStats() const;
  std::size_t entryCount() const;

 private:
  void finalizeStatsUnlocked() const;

  std::size_t max_entries_ = 0;
  mutable std::mutex mutex_;
  mutable SDFSampleCacheStats stats_;
  std::unordered_map<QuantizedPointKey, CachedSDFSample, QuantizedPointKeyHash>
      samples_;
};

}  // namespace adasdf
