#include "adasdf/lookup/ActiveBlockHashMap.h"

#include <algorithm>
#include <chrono>

namespace adasdf {

void ActiveBlockHashMap::rebuild(const std::vector<ActiveBlockEntry>& entries) {
  const auto start = std::chrono::steady_clock::now();
  block_to_slot_.clear();
  sorted_entries_.clear();
  for (const ActiveBlockEntry& entry : entries) {
    if (entry.block_id < 0 || entry.cache_slot < 0) {
      continue;
    }
    block_to_slot_[entry.block_id] = entry.cache_slot;
    sorted_entries_.push_back({entry.block_id, entry.cache_slot});
  }
  std::sort(sorted_entries_.begin(), sorted_entries_.end());
  stats_ = {};
  stats_.active_count = block_to_slot_.size();
  stats_.build_time_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
}

int ActiveBlockHashMap::findCacheSlot(int block_id) const {
  const auto start = std::chrono::steady_clock::now();
  ++stats_.lookup_count;
  const auto it = block_to_slot_.find(block_id);
  if (it != block_to_slot_.end()) {
    ++stats_.hit_count;
    stats_.hit_rate =
        static_cast<double>(stats_.hit_count) /
        static_cast<double>(stats_.lookup_count);
    stats_.lookup_time_ms +=
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();
    return it->second;
  }
  ++stats_.miss_count;
  stats_.hit_rate =
      static_cast<double>(stats_.hit_count) /
      static_cast<double>(stats_.lookup_count);
  stats_.lookup_time_ms +=
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  return -1;
}

bool ActiveBlockHashMap::contains(int block_id) const {
  return findCacheSlot(block_id) >= 0;
}

void ActiveBlockHashMap::clear() {
  block_to_slot_.clear();
  sorted_entries_.clear();
  stats_ = {};
}

const ActiveBlockHashMapStats& ActiveBlockHashMap::stats() const {
  return stats_;
}

}  // namespace adasdf
