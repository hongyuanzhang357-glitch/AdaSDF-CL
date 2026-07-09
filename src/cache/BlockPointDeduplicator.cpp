#include "adasdf/cache/BlockPointDeduplicator.h"

#include <unordered_map>

namespace adasdf {

DeduplicatedPointSet BlockPointDeduplicator::deduplicate(
    const std::vector<Vector3>& points,
    int level,
    const QuantizationOptions& quantization) {
  DeduplicatedPointSet result;
  result.reverse_index.reserve(points.size());
  result.unique_points.reserve(points.size());
  result.keys.reserve(points.size());
  std::unordered_map<QuantizedPointKey, int, QuantizedPointKeyHash> seen;
  for (const Vector3& point : points) {
    const QuantizedPointKey key =
        QuantizedPointKeyBuilder::fromPoint(point, level, quantization);
    const auto it = seen.find(key);
    if (it != seen.end()) {
      result.reverse_index.push_back(it->second);
      ++result.duplicate_count;
      continue;
    }
    const int index = static_cast<int>(result.unique_points.size());
    seen.emplace(key, index);
    result.unique_points.push_back(point);
    result.keys.push_back(key);
    result.reverse_index.push_back(index);
  }
  return result;
}

}  // namespace adasdf
