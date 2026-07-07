#pragma once

#include <cstddef>
#include <list>
#include <unordered_map>
#include <vector>

#include "adasdf/runtime/ExpandedBlock.h"

namespace adasdf {

struct ExpandedBlockCacheOptions {
  std::size_t max_blocks = 64;
  std::size_t max_memory_bytes = 256ull * 1024ull * 1024ull;
  bool enable_lru = true;
};

struct ExpandedBlockCacheStats {
  std::size_t request_count = 0;
  std::size_t hit_count = 0;
  std::size_t miss_count = 0;
  std::size_t insert_count = 0;
  std::size_t eviction_count = 0;
  std::size_t block_count = 0;
  std::size_t memory_bytes = 0;

  double hitRate() const;
};

class ExpandedBlockCache {
 public:
  explicit ExpandedBlockCache(
      const ExpandedBlockCacheOptions& options =
          ExpandedBlockCacheOptions{});

  const ExpandedBlockCacheOptions& options() const;
  bool contains(int block_id) const;
  const ActiveExpandedBlock* get(int block_id);
  const ActiveExpandedBlock* peek(int block_id) const;
  void put(ActiveExpandedBlock block);
  void clear();
  std::vector<int> residentBlockIds() const;
  std::vector<ActiveExpandedBlock> residentBlocks() const;
  ExpandedBlockCacheStats stats() const;

 private:
  struct Entry {
    ActiveExpandedBlock block;
    std::list<int>::iterator lru_it;
  };

  void touch(Entry& entry);
  void evictIfNeeded();
  std::unordered_map<int, Entry> entries_;
  std::list<int> lru_;
  ExpandedBlockCacheOptions options_;
  ExpandedBlockCacheStats stats_;
};

}  // namespace adasdf
