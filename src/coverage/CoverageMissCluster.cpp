#include "adasdf/coverage/CoverageMissCluster.h"

#include <algorithm>
#include <utility>

namespace adasdf {

std::vector<int> CoverageMissCluster::missedBlocksAboveThreshold(
    const CoverageAuditResult& audit,
    std::size_t min_miss_count) {
  std::vector<std::pair<int, std::size_t>> pairs;
  for (const auto& item : audit.missed_by_block_id) {
    if (item.first >= 0 && item.second >= min_miss_count) {
      pairs.push_back(item);
    }
  }
  std::sort(
      pairs.begin(),
      pairs.end(),
      [](const auto& a, const auto& b) {
        if (a.second != b.second) {
          return a.second > b.second;
        }
        return a.first < b.first;
      });
  std::vector<int> out;
  out.reserve(pairs.size());
  for (const auto& item : pairs) {
    out.push_back(item.first);
  }
  return out;
}

}  // namespace adasdf
