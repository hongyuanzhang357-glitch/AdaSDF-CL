#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/geometry/Transform.h"
#include "adasdf/lookup/BlockSpatialKey.h"

namespace adasdf {

enum class BlockLookupMode {
  LinearScan,
  SpatialHash,
  MortonSorted
};

const char* toString(BlockLookupMode mode);
BlockLookupMode parseBlockLookupMode(const std::string& text);

struct BlockLookupIndexOptions {
  BlockLookupMode mode = BlockLookupMode::SpatialHash;
  bool use_level_aware_keys = true;
  bool allow_linear_fallback = true;
  double aabb_epsilon = 1.0e-12;
};

struct BlockLookupResult {
  bool found = false;
  int block_id = -1;
  int candidate_count = 0;
  bool used_fallback = false;
};

struct BlockLookupIndexStats {
  std::size_t block_count = 0;
  std::size_t bucket_count = 0;
  std::size_t max_bucket_size = 0;
  double average_bucket_size = 0.0;
  double build_time_ms = 0.0;
  std::size_t query_count = 0;
  std::size_t linear_fallback_count = 0;
  std::size_t missed_lookup_count = 0;
  double query_time_ms = 0.0;
};

class BlockLookupIndex {
 public:
  bool buildFromAdaptiveBlocks(
      const std::vector<AdaptiveSDFBlock>& blocks,
      const AABB& domain,
      const BlockLookupIndexOptions& options);

  BlockLookupResult findBlockContainingPoint(const Vector3& p) const;
  const BlockLookupIndexStats& stats() const;
  void clear();

 private:
  struct BlockRecord {
    int block_id = -1;
    int level = 0;
    AABB bounds;
    double diagonal = 0.0;
  };

  struct BucketEntry {
    BlockSpatialKey key;
    int block_index = -1;
  };

  BlockLookupResult linearFind(const Vector3& p, bool fallback) const;
  BlockLookupResult bucketFind(
      const Vector3& p,
      const std::vector<int>& candidates) const;
  BlockSpatialKey keyForPoint(const Vector3& p) const;
  std::vector<int> candidatesForKey(const BlockSpatialKey& key) const;

  std::vector<BlockRecord> blocks_;
  AABB domain_;
  BlockLookupIndexOptions options_;
  Vector3 cell_size_{1.0, 1.0, 1.0};
  int key_level_ = 0;
  std::unordered_map<BlockSpatialKey, std::vector<int>, BlockSpatialKeyHash>
      buckets_;
  std::vector<BucketEntry> sorted_entries_;
  std::vector<int> overflow_blocks_;
  mutable BlockLookupIndexStats stats_;
};

}  // namespace adasdf
