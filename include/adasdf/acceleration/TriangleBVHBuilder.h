#pragma once

#include "adasdf/acceleration/TriangleBVH.h"

namespace adasdf {

class TriangleBVHBuilder {
 public:
  static TriangleBVH build(
      const TriangleMesh& mesh,
      const TriangleBVHBuildOptions& options = TriangleBVHBuildOptions(),
      TriangleBVHBuildReport* report = nullptr);
};

}  // namespace adasdf
