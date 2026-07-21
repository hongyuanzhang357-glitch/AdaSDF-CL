#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/coverage/ContactBandCoverageAudit.h"

namespace adasdf {

class CoverageMissCluster {
 public:
  static std::vector<int> missedBlocksAboveThreshold(
      const CoverageAuditResult& audit,
      std::size_t min_miss_count);
};

}  // namespace adasdf
