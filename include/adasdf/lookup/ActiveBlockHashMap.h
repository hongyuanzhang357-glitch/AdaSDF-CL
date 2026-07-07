#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace adasdf {

struct ActiveBlockEntry {
  int block_id = -1;
  int cache_slot = -1;
  std::uint64_t generation = 0;
  bool resident = false;
};

struct ActiveBlockHashMapStats {
  std::size_t active_count = 0;
  std::size_t lookup_count = 0;
  std::size_t hit_count = 0;
  std::size_t miss_count = 0;
  double hit_rate = 0.0;
  double build_time_ms = 0.0;
  double lookup_time_ms = 0.0;
};

class ActiveBlockHashMap {
 public:
  void rebuild(const std::vector<ActiveBlockEntry>& entries);
  int findCacheSlot(int block_id) const;
  bool contains(int block_id) const;
  void clear();
  const ActiveBlockHashMapStats& stats() const;

 private:
  mutable ActiveBlockHashMapStats stats_;
  std::unordered_map<int, int> block_to_slot_;
  std::vector<std::pair<int, int>> sorted_entries_;
};

}  // namespace adasdf
