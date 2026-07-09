#pragma once

#include <cstddef>
#include <cstdint>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct QuantizedPointKey {
  std::int64_t ix = 0;
  std::int64_t iy = 0;
  std::int64_t iz = 0;
  int level = 0;

  bool operator==(const QuantizedPointKey& other) const noexcept;
};

struct QuantizedPointKeyHash {
  std::size_t operator()(const QuantizedPointKey& key) const noexcept;
};

struct QuantizationOptions {
  Vector3 origin;
  double spacing = 1.0;
  double epsilon = 1e-12;
  bool include_level = true;
};

class QuantizedPointKeyBuilder {
 public:
  static QuantizedPointKey fromPoint(
      const Vector3& p,
      int level,
      const QuantizationOptions& options);
};

}  // namespace adasdf
