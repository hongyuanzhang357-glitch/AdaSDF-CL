#pragma once

#include <unordered_map>
#include <vector>

#include "adasdf/lookup/ActiveBlockHashMap.h"
#include "adasdf/lookup/BlockLookupIndex.h"
#include "adasdf/runtime/ExpandedBlock.h"

namespace adasdf {

class ActiveBlockSpatialHash {
 public:
  bool build(
      const std::vector<ActiveExpandedBlock>& active_blocks,
      const AABB& domain,
      const BlockLookupIndexOptions& options);

  BlockLookupResult findActiveBlockContainingPoint(const Vector3& p) const;
  int findCacheSlotForPoint(const Vector3& p) const;
  const BlockLookupIndexStats& stats() const;
  void clear();

 private:
  struct BlockRecord {
    int block_id = -1;
    int cache_slot = -1;
    int level = 0;
    AABB bounds;
    double diagonal = 0.0;
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
  std::vector<std::pair<BlockSpatialKey, int>> sorted_entries_;
  std::vector<int> overflow_blocks_;
  ActiveBlockHashMap block_id_to_slot_;
  mutable BlockLookupIndexStats stats_;
};

}  // namespace adasdf
