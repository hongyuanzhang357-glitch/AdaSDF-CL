#pragma once

#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct DifferentialContactData {
  Vector3 gradient_a;
  Vector3 gradient_b;
  MatrixX jacobian_a;
  MatrixX jacobian_b;

  // TODO: connect to autodiff / analytic gradient / CUDA differentiable kernels.
};

struct Contact {
  Vector3 point;
  Vector3 normal;
  Scalar penetration_depth = 0.0;
  Scalar signed_distance = 0.0;

  ObjectId object_id_a = -1;
  ObjectId object_id_b = -1;
  FeatureId feature_id_a = -1;
  FeatureId feature_id_b = -1;

  Vector3 gradient;
  DifferentialContactData differential;
};

using ContactList = std::vector<Contact>;

}  // namespace adasdf
