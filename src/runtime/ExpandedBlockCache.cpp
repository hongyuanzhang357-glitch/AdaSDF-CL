#include "adasdf/runtime/ExpandedBlockCache.h"

#include <algorithm>

namespace adasdf {

double ExpandedBlockCacheStats::hitRate() const {
  return request_count > 0
      ? static_cast<double>(hit_count) / static_cast<double>(request_count)
      : 0.0;
}

ExpandedBlockCache::ExpandedBlockCache(
    const ExpandedBlockCacheOptions& options)
    : options_(options) {}

const ExpandedBlockCacheOptions& ExpandedBlockCache::options() const {
  return options_;
}

bool ExpandedBlockCache::contains(int block_id) const {
  return entries_.find(block_id) != entries_.end();
}

const ActiveExpandedBlock* ExpandedBlockCache::get(int block_id) {
  ++stats_.request_count;
  auto it = entries_.find(block_id);
  if (it == entries_.end()) {
    ++stats_.miss_count;
    return nullptr;
  }
  ++stats_.hit_count;
  touch(it->second);
  return &it->second.block;
}

const ActiveExpandedBlock* ExpandedBlockCache::peek(int block_id) const {
  auto it = entries_.find(block_id);
  return it == entries_.end() ? nullptr : &it->second.block;
}

void ExpandedBlockCache::put(ActiveExpandedBlock block) {
  if (!block.isValid()) {
    return;
  }

  const int block_id = block.block_id;
  auto existing = entries_.find(block_id);
  if (existing != entries_.end()) {
    stats_.memory_bytes -= existing->second.block.memoryBytes();
    lru_.erase(existing->second.lru_it);
    entries_.erase(existing);
  }

  lru_.push_front(block_id);
  Entry entry;
  entry.block = std::move(block);
  entry.lru_it = lru_.begin();
  stats_.memory_bytes += entry.block.memoryBytes();
  entries_.emplace(block_id, std::move(entry));
  ++stats_.insert_count;
  stats_.block_count = entries_.size();
  evictIfNeeded();
}

void ExpandedBlockCache::clear() {
  entries_.clear();
  lru_.clear();
  stats_.block_count = 0;
  stats_.memory_bytes = 0;
}

std::vector<int> ExpandedBlockCache::residentBlockIds() const {
  std::vector<int> ids;
  ids.reserve(entries_.size());
  for (const auto& item : entries_) {
    ids.push_back(item.first);
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

std::vector<ActiveExpandedBlock> ExpandedBlockCache::residentBlocks() const {
  std::vector<ActiveExpandedBlock> blocks;
  blocks.reserve(entries_.size());
  for (const auto& item : entries_) {
    blocks.push_back(item.second.block);
  }
  std::sort(
      blocks.begin(),
      blocks.end(),
      [](const ActiveExpandedBlock& a, const ActiveExpandedBlock& b) {
        return a.block_id < b.block_id;
      });
  return blocks;
}

ExpandedBlockCacheStats ExpandedBlockCache::stats() const {
  ExpandedBlockCacheStats copy = stats_;
  copy.block_count = entries_.size();
  return copy;
}

void ExpandedBlockCache::touch(Entry& entry) {
  if (!options_.enable_lru) {
    return;
  }
  const int block_id = entry.block.block_id;
  lru_.erase(entry.lru_it);
  lru_.push_front(block_id);
  entry.lru_it = lru_.begin();
}

void ExpandedBlockCache::evictIfNeeded() {
  if (!options_.enable_lru) {
    stats_.block_count = entries_.size();
    return;
  }
  while (entries_.size() > 1 &&
         ((options_.max_blocks > 0 && entries_.size() > options_.max_blocks) ||
          (options_.max_memory_bytes > 0 &&
           stats_.memory_bytes > options_.max_memory_bytes))) {
    const int victim = lru_.back();
    lru_.pop_back();
    auto it = entries_.find(victim);
    if (it != entries_.end()) {
      stats_.memory_bytes -= it->second.block.memoryBytes();
      entries_.erase(it);
      ++stats_.eviction_count;
    }
  }
  stats_.block_count = entries_.size();
}

}  // namespace adasdf
