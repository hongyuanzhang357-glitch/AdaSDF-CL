#pragma once

#include "adasdf/geometry/Transform.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct MeshSignOptions {
  double ray_epsilon = 1e-12;
  int max_retry_rays = 3;
};

enum class MeshSignResult {
  Outside,
  Inside,
  OnSurface,
  Ambiguous
};

class MeshSign {
 public:
  static MeshSignResult classifyPoint(
      const TriangleMesh& mesh,
      const Vector3& p,
      const MeshSignOptions& options = MeshSignOptions{});
};

const char* toString(MeshSignResult result);

}  // namespace adasdf
