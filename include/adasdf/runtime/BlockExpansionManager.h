#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/runtime/ExpandedBlockCache.h"

namespace adasdf {

struct BlockExpansionStats {
  std::size_t requested_block_count = 0;
  std::size_t expanded_block_count = 0;
  std::size_t cache_hit_count = 0;
  std::size_t cache_miss_count = 0;
  std::size_t skipped_block_count = 0;
  std::size_t expanded_memory_bytes = 0;
  double expansion_time_ms = 0.0;
};

struct BlockExpansionResult {
  bool success = false;
  std::string error_message;
  std::vector<int> resident_block_ids;
  BlockExpansionStats stats;
  ExpandedBlockCacheStats cache_stats;
  std::vector<std::string> warnings;
};

class BlockExpansionManager {
 public:
  explicit BlockExpansionManager(ExpandedBlockCache* cache);

  BlockExpansionResult ensureBlocksExpanded(
      const SDFModel& model,
      const std::vector<int>& block_ids);

  ActiveExpandedBlock expandBlock(
      const SDFModel& model,
      int block_id,
      std::string* error_message = nullptr) const;

 private:
  ExpandedBlockCache* cache_ = nullptr;
};

}  // namespace adasdf
