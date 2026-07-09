#pragma once

#include <cstddef>
#include <mutex>
#include <unordered_map>

#include "adasdf/cache/QuantizedPointKey.h"

namespace adasdf {

struct CellCornerCacheStats {
  std::size_t lookup_count = 0;
  std::size_t hit_count = 0;
  std::size_t miss_count = 0;
  std::size_t insert_count = 0;
  std::size_t entry_count = 0;
  double hit_rate = 0.0;
};

class CellCornerCache {
 public:
  bool findCorner(const QuantizedPointKey& key, double* phi) const;
  void insertCorner(const QuantizedPointKey& key, double phi);
  void clearBlockLocal();
  void clearGlobal();
  const CellCornerCacheStats& stats() const;
  CellCornerCacheStats snapshotStats() const;

 private:
  void finalizeStatsUnlocked() const;

  mutable std::mutex mutex_;
  mutable CellCornerCacheStats stats_;
  std::unordered_map<QuantizedPointKey, double, QuantizedPointKeyHash> corners_;
};

}  // namespace adasdf
