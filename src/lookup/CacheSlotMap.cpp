#include "adasdf/lookup/CacheSlotMap.h"

#include <algorithm>

namespace adasdf {

void CacheSlotMap::rebuildFromActiveBlocks(
    const std::vector<ActiveExpandedBlock>& blocks) {
  BlockLookupIndexOptions options;
  options.mode = BlockLookupMode::SpatialHash;
  options.allow_linear_fallback = true;
  rebuildFromActiveBlocks(blocks, options);
}

void CacheSlotMap::rebuildFromActiveBlocks(
    const std::vector<ActiveExpandedBlock>& blocks,
    const BlockLookupIndexOptions& options) {
  std::vector<ActiveBlockEntry> entries;
  entries.reserve(blocks.size());
  AABB domain;
  bool domain_initialized = false;
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    const ActiveExpandedBlock& block = blocks[i];
    if (!block.isValid()) {
      continue;
    }
    entries.push_back({block.block_id, static_cast<int>(i), 0, true});
    if (!domain_initialized) {
      domain = block.bounds;
      domain_initialized = true;
    } else {
      domain.min.x = std::min(domain.min.x, block.bounds.min.x);
      domain.min.y = std::min(domain.min.y, block.bounds.min.y);
      domain.min.z = std::min(domain.min.z, block.bounds.min.z);
      domain.max.x = std::max(domain.max.x, block.bounds.max.x);
      domain.max.y = std::max(domain.max.y, block.bounds.max.y);
      domain.max.z = std::max(domain.max.z, block.bounds.max.z);
      domain.valid = true;
    }
  }
  block_id_map_.rebuild(entries);
  spatial_hash_.build(blocks, domain, options);
}

int CacheSlotMap::blockIdToSlot(int block_id) const {
  return block_id_map_.findCacheSlot(block_id);
}

int CacheSlotMap::pointToSlot(const Vector3& p) const {
  const BlockLookupResult result = spatial_hash_.findActiveBlockContainingPoint(p);
  return result.found ? block_id_map_.findCacheSlot(result.block_id) : -1;
}

bool CacheSlotMap::containsBlock(int block_id) const {
  return block_id_map_.contains(block_id);
}

const ActiveBlockHashMapStats& CacheSlotMap::blockIdStats() const {
  return block_id_map_.stats();
}

const BlockLookupIndexStats& CacheSlotMap::spatialStats() const {
  return spatial_hash_.stats();
}

}  // namespace adasdf
