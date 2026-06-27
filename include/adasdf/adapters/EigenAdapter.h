#pragma once

#include "adasdf/geometry/Transform.h"

namespace adasdf {

// Optional adapter surface. Keep Eigen out of the public core until the build
// contract is finalized, then map Vector3/Transform to Eigen types here.
struct EigenAdapterPlan {
  bool expose_vector_map = true;
  bool expose_transform_map = true;
  bool keep_core_dependency_free = true;
};

}  // namespace adasdf
