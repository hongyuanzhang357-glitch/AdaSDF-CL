#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/cache/QuantizedPointKey.h"

namespace adasdf {

struct DeduplicatedPointSet {
  std::vector<Vector3> unique_points;
  std::vector<QuantizedPointKey> keys;
  std::vector<int> reverse_index;
  std::size_t duplicate_count = 0;
};

class BlockPointDeduplicator {
 public:
  static DeduplicatedPointSet deduplicate(
      const std::vector<Vector3>& points,
      int level,
      const QuantizationOptions& quantization);
};

}  // namespace adasdf
