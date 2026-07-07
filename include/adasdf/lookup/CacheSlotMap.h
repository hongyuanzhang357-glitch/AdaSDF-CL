#pragma once

#include <vector>

#include "adasdf/lookup/ActiveBlockHashMap.h"
#include "adasdf/lookup/ActiveBlockSpatialHash.h"
#include "adasdf/runtime/ExpandedBlock.h"

namespace adasdf {

class CacheSlotMap {
 public:
  void rebuildFromActiveBlocks(const std::vector<ActiveExpandedBlock>& blocks);
  void rebuildFromActiveBlocks(
      const std::vector<ActiveExpandedBlock>& blocks,
      const BlockLookupIndexOptions& options);

  int blockIdToSlot(int block_id) const;
  int pointToSlot(const Vector3& p) const;
  bool containsBlock(int block_id) const;

  const ActiveBlockHashMapStats& blockIdStats() const;
  const BlockLookupIndexStats& spatialStats() const;

 private:
  ActiveBlockHashMap block_id_map_;
  ActiveBlockSpatialHash spatial_hash_;
  BlockLookupIndexStats empty_spatial_stats_;
};

}  // namespace adasdf
