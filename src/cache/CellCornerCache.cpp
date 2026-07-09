#include "adasdf/cache/CellCornerCache.h"

namespace adasdf {

bool CellCornerCache::findCorner(
    const QuantizedPointKey& key,
    double* phi) const {
  std::lock_guard<std::mutex> lock(mutex_);
  ++stats_.lookup_count;
  const auto it = corners_.find(key);
  if (it == corners_.end()) {
    ++stats_.miss_count;
    finalizeStatsUnlocked();
    return false;
  }
  ++stats_.hit_count;
  if (phi != nullptr) {
    *phi = it->second;
  }
  finalizeStatsUnlocked();
  return true;
}

void CellCornerCache::insertCorner(
    const QuantizedPointKey& key,
    double phi) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto inserted = corners_.emplace(key, phi);
  if (!inserted.second) {
    inserted.first->second = phi;
  } else {
    ++stats_.insert_count;
  }
  finalizeStatsUnlocked();
}

void CellCornerCache::clearBlockLocal() {
  clearGlobal();
}

void CellCornerCache::clearGlobal() {
  std::lock_guard<std::mutex> lock(mutex_);
  corners_.clear();
  stats_ = {};
}

const CellCornerCacheStats& CellCornerCache::stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  finalizeStatsUnlocked();
  return stats_;
}

CellCornerCacheStats CellCornerCache::snapshotStats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  finalizeStatsUnlocked();
  return stats_;
}

void CellCornerCache::finalizeStatsUnlocked() const {
  stats_.entry_count = corners_.size();
  stats_.hit_rate =
      stats_.lookup_count > 0
          ? static_cast<double>(stats_.hit_count) /
                static_cast<double>(stats_.lookup_count)
          : 0.0;
}

}  // namespace adasdf
