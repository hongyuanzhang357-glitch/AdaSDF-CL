#pragma once

#include "adasdf/geometry/Transform.h"

namespace adasdf {

double pointTriangleSquaredDistance(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c);

double pointTriangleDistance(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c);

}  // namespace adasdf
