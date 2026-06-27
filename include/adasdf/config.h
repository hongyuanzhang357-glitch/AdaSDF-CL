#pragma once

#include <cstddef>
#include <cstdint>

namespace adasdf {

using Scalar = double;
using ObjectId = std::int64_t;
using FeatureId = std::int64_t;

enum class BackendType {
  CPU,
  CUDA,
  Auto
};

enum class QueryMode {
  Fast,
  Balanced,
  Accurate
};

enum class BroadphaseType {
  None,
  AABB,
  BVH,
  Auto
};

enum class NarrowphaseType {
  SDFSampling,
  AdaptiveSDF,
  TriangleFallback,
  Auto
};

struct LibraryVersion {
  int major = 0;
  int minor = 6;
  int patch = 0;
  const char* label = "alpha";
};

inline constexpr LibraryVersion version() {
  return {};
}

}  // namespace adasdf
