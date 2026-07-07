#include "adasdf/lookup/BlockLookupIndex.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace adasdf {
namespace {

double axisExtent(double min_value, double max_value) {
  const double extent = max_value - min_value;
  return extent > 0.0 && std::isfinite(extent) ? extent : 1.0;
}

double diagonalLength(const AABB& box) {
  const Vector3 d = box.max - box.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

bool containsPoint(const AABB& box, const Vector3& p, double eps) {
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

int axisCell(double value, double origin, double cell_size) {
  if (!(cell_size > 0.0) || !std::isfinite(cell_size)) {
    return 0;
  }
  return static_cast<int>(std::floor((value - origin) / cell_size));
}

void updateBucketStats(
    const std::unordered_map<BlockSpatialKey, std::vector<int>, BlockSpatialKeyHash>&
        buckets,
    BlockLookupIndexStats& stats) {
  stats.bucket_count = buckets.size();
  stats.max_bucket_size = 0;
  std::size_t total = 0;
  for (const auto& item : buckets) {
    stats.max_bucket_size = std::max(stats.max_bucket_size, item.second.size());
    total += item.second.size();
  }
  stats.average_bucket_size =
      buckets.empty()
          ? 0.0
          : static_cast<double>(total) / static_cast<double>(buckets.size());
}

}  // namespace

const char* toString(BlockLookupMode mode) {
  switch (mode) {
    case BlockLookupMode::LinearScan:
      return "linear";
    case BlockLookupMode::SpatialHash:
      return "hash";
    case BlockLookupMode::MortonSorted:
      return "morton";
  }
  return "linear";
}

BlockLookupMode parseBlockLookupMode(const std::string& text) {
  if (text == "linear" || text == "linear-scan") {
    return BlockLookupMode::LinearScan;
  }
  if (text == "hash" || text == "spatial-hash") {
    return BlockLookupMode::SpatialHash;
  }
  if (text == "morton" || text == "morton-sorted") {
    return BlockLookupMode::MortonSorted;
  }
  throw std::runtime_error("unsupported block lookup mode: " + text);
}

bool BlockLookupIndex::buildFromAdaptiveBlocks(
    const std::vector<AdaptiveSDFBlock>& blocks,
    const AABB& domain,
    const BlockLookupIndexOptions& options) {
  clear();
  const auto start = std::chrono::steady_clock::now();
  options_ = options;
  domain_ = domain;
  domain_.valid = domain_.valid;

  AABB computed_domain;
  double min_x = std::numeric_limits<double>::infinity();
  double min_y = std::numeric_limits<double>::infinity();
  double min_z = std::numeric_limits<double>::infinity();
  double max_x = -std::numeric_limits<double>::infinity();
  double max_y = -std::numeric_limits<double>::infinity();
  double max_z = -std::numeric_limits<double>::infinity();
  double cell_x = std::numeric_limits<double>::infinity();
  double cell_y = std::numeric_limits<double>::infinity();
  double cell_z = std::numeric_limits<double>::infinity();

  for (const AdaptiveSDFBlock& block : blocks) {
    if (block.block_id < 0 || !block.bounds.valid ||
        !block.bounds.min.allFinite() || !block.bounds.max.allFinite()) {
      continue;
    }
    BlockRecord record;
    record.block_id = block.block_id;
    record.level = block.level;
    record.bounds = block.bounds;
    record.diagonal = diagonalLength(block.bounds);
    blocks_.push_back(record);
    min_x = std::min(min_x, block.bounds.min.x);
    min_y = std::min(min_y, block.bounds.min.y);
    min_z = std::min(min_z, block.bounds.min.z);
    max_x = std::max(max_x, block.bounds.max.x);
    max_y = std::max(max_y, block.bounds.max.y);
    max_z = std::max(max_z, block.bounds.max.z);
    cell_x = std::min(cell_x, axisExtent(block.bounds.min.x, block.bounds.max.x));
    cell_y = std::min(cell_y, axisExtent(block.bounds.min.y, block.bounds.max.y));
    cell_z = std::min(cell_z, axisExtent(block.bounds.min.z, block.bounds.max.z));
    key_level_ = std::max(key_level_, block.level);
  }

  if (blocks_.empty()) {
    return false;
  }

  computed_domain.min = {min_x, min_y, min_z};
  computed_domain.max = {max_x, max_y, max_z};
  computed_domain.valid = true;
  if (!domain_.valid) {
    domain_ = computed_domain;
  }
  cell_size_ = {
      axisExtent(0.0, cell_x),
      axisExtent(0.0, cell_y),
      axisExtent(0.0, cell_z)};
  key_level_ = options_.use_level_aware_keys ? key_level_ : 0;

  for (std::size_t index = 0; index < blocks_.size(); ++index) {
    const BlockRecord& block = blocks_[index];
    const int ix0 = axisCell(
        block.bounds.min.x - options_.aabb_epsilon,
        domain_.min.x,
        cell_size_.x);
    const int iy0 = axisCell(
        block.bounds.min.y - options_.aabb_epsilon,
        domain_.min.y,
        cell_size_.y);
    const int iz0 = axisCell(
        block.bounds.min.z - options_.aabb_epsilon,
        domain_.min.z,
        cell_size_.z);
    const int ix1 = axisCell(
        block.bounds.max.x + options_.aabb_epsilon,
        domain_.min.x,
        cell_size_.x);
    const int iy1 = axisCell(
        block.bounds.max.y + options_.aabb_epsilon,
        domain_.min.y,
        cell_size_.y);
    const int iz1 = axisCell(
        block.bounds.max.z + options_.aabb_epsilon,
        domain_.min.z,
        cell_size_.z);
    const long long cell_count =
        static_cast<long long>(ix1 - ix0 + 1) *
        static_cast<long long>(iy1 - iy0 + 1) *
        static_cast<long long>(iz1 - iz0 + 1);
    if (cell_count <= 0 || cell_count > 4096) {
      overflow_blocks_.push_back(static_cast<int>(index));
      continue;
    }
    for (int z = iz0; z <= iz1; ++z) {
      for (int y = iy0; y <= iy1; ++y) {
        for (int x = ix0; x <= ix1; ++x) {
          const BlockSpatialKey key = makeBlockSpatialKey({x, y, z}, key_level_);
          buckets_[key].push_back(static_cast<int>(index));
          sorted_entries_.push_back({key, static_cast<int>(index)});
        }
      }
    }
  }

  std::sort(
      sorted_entries_.begin(),
      sorted_entries_.end(),
      [](const BucketEntry& a, const BucketEntry& b) {
        if (a.key == b.key) {
          return a.block_index < b.block_index;
        }
        return a.key < b.key;
      });
  stats_.block_count = blocks_.size();
  updateBucketStats(buckets_, stats_);
  stats_.build_time_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  return true;
}

BlockLookupResult BlockLookupIndex::findBlockContainingPoint(
    const Vector3& p) const {
  const auto start = std::chrono::steady_clock::now();
  BlockLookupResult result;
  if (options_.mode == BlockLookupMode::LinearScan) {
    result = linearFind(p, false);
  } else {
    const std::vector<int> candidates = candidatesForKey(keyForPoint(p));
    result = bucketFind(p, candidates);
    if (!result.found && options_.allow_linear_fallback) {
      result = linearFind(p, true);
    }
  }
  ++stats_.query_count;
  if (result.used_fallback) {
    ++stats_.linear_fallback_count;
  }
  if (!result.found) {
    ++stats_.missed_lookup_count;
  }
  stats_.query_time_ms +=
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  return result;
}

const BlockLookupIndexStats& BlockLookupIndex::stats() const {
  return stats_;
}

void BlockLookupIndex::clear() {
  blocks_.clear();
  buckets_.clear();
  sorted_entries_.clear();
  overflow_blocks_.clear();
  stats_ = {};
  domain_ = {};
  cell_size_ = {1.0, 1.0, 1.0};
  key_level_ = 0;
}

BlockLookupResult BlockLookupIndex::linearFind(
    const Vector3& p,
    bool fallback) const {
  BlockLookupResult result;
  result.used_fallback = fallback;
  int best_index = -1;
  int best_level = std::numeric_limits<int>::min();
  double best_diag = std::numeric_limits<double>::infinity();
  int best_block_id = std::numeric_limits<int>::max();
  for (std::size_t i = 0; i < blocks_.size(); ++i) {
    const BlockRecord& block = blocks_[i];
    if (!containsPoint(block.bounds, p, options_.aabb_epsilon)) {
      continue;
    }
    ++result.candidate_count;
    if (block.level > best_level ||
        (block.level == best_level && block.diagonal < best_diag) ||
        (block.level == best_level && block.diagonal == best_diag &&
         block.block_id < best_block_id)) {
      best_index = static_cast<int>(i);
      best_level = block.level;
      best_diag = block.diagonal;
      best_block_id = block.block_id;
    }
  }
  if (best_index >= 0) {
    result.found = true;
    result.block_id = blocks_[static_cast<std::size_t>(best_index)].block_id;
  }
  return result;
}

BlockLookupResult BlockLookupIndex::bucketFind(
    const Vector3& p,
    const std::vector<int>& candidates) const {
  BlockLookupResult result;
  int best_index = -1;
  int best_level = std::numeric_limits<int>::min();
  double best_diag = std::numeric_limits<double>::infinity();
  int best_block_id = std::numeric_limits<int>::max();
  for (const int index : candidates) {
    if (index < 0 || static_cast<std::size_t>(index) >= blocks_.size()) {
      continue;
    }
    const BlockRecord& block = blocks_[static_cast<std::size_t>(index)];
    if (!containsPoint(block.bounds, p, options_.aabb_epsilon)) {
      continue;
    }
    ++result.candidate_count;
    if (block.level > best_level ||
        (block.level == best_level && block.diagonal < best_diag) ||
        (block.level == best_level && block.diagonal == best_diag &&
         block.block_id < best_block_id)) {
      best_index = index;
      best_level = block.level;
      best_diag = block.diagonal;
      best_block_id = block.block_id;
    }
  }
  if (best_index >= 0) {
    result.found = true;
    result.block_id = blocks_[static_cast<std::size_t>(best_index)].block_id;
  }
  return result;
}

BlockSpatialKey BlockLookupIndex::keyForPoint(const Vector3& p) const {
  return makeBlockSpatialKey(
      {axisCell(p.x, domain_.min.x, cell_size_.x),
       axisCell(p.y, domain_.min.y, cell_size_.y),
       axisCell(p.z, domain_.min.z, cell_size_.z)},
      key_level_);
}

std::vector<int> BlockLookupIndex::candidatesForKey(
    const BlockSpatialKey& key) const {
  std::vector<int> candidates = overflow_blocks_;
  if (options_.mode == BlockLookupMode::SpatialHash) {
    const auto it = buckets_.find(key);
    if (it != buckets_.end()) {
      candidates.insert(candidates.end(), it->second.begin(), it->second.end());
    }
    return candidates;
  }

  const auto lower = std::lower_bound(
      sorted_entries_.begin(),
      sorted_entries_.end(),
      key,
      [](const BucketEntry& item, const BlockSpatialKey& wanted) {
        return item.key < wanted;
      });
  for (auto it = lower; it != sorted_entries_.end() &&
                         !(key < it->key) && !(it->key < key);
       ++it) {
    candidates.push_back(it->block_index);
  }
  return candidates;
}

}  // namespace adasdf
