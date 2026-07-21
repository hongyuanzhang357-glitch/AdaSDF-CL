#include "adasdf/audit/LocalExactSubcellProbe.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

int signWithEps(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double axisT(double p, double lo, double hi) {
  if (!(hi > lo)) {
    return 0.0;
  }
  return std::clamp((p - lo) / (hi - lo), 0.0, 1.0);
}

Vector3 pointInCell(const AABB& box, double u, double v, double w) {
  return {
      lerp(box.min.x, box.max.x, u),
      lerp(box.min.y, box.max.y, v),
      lerp(box.min.z, box.max.z, w)};
}

}  // namespace

LocalExactSubcellProbeResult LocalExactSubcellProbe::probe(
    const AABB& cell_bounds,
    const Vector3& query_point,
    int original_sdf_sign,
    int reference_sign,
    int subgrid,
    const ExactPhiFunction& exact_phi,
    double sign_epsilon) {
  LocalExactSubcellProbeResult result;
  result.subgrid = subgrid;
  if (!cell_bounds.valid || subgrid <= 0 || !exact_phi) {
    result.remaining_mismatch = original_sdf_sign != reference_sign;
    result.sign = original_sdf_sign;
    return result;
  }

  const double gx =
      axisT(query_point.x, cell_bounds.min.x, cell_bounds.max.x) *
      static_cast<double>(subgrid);
  const double gy =
      axisT(query_point.y, cell_bounds.min.y, cell_bounds.max.y) *
      static_cast<double>(subgrid);
  const double gz =
      axisT(query_point.z, cell_bounds.min.z, cell_bounds.max.z) *
      static_cast<double>(subgrid);
  const int i = std::max(
      0,
      std::min(subgrid - 1, static_cast<int>(std::floor(gx))));
  const int j = std::max(
      0,
      std::min(subgrid - 1, static_cast<int>(std::floor(gy))));
  const int k = std::max(
      0,
      std::min(subgrid - 1, static_cast<int>(std::floor(gz))));
  const double tx = gx - static_cast<double>(i);
  const double ty = gy - static_cast<double>(j);
  const double tz = gz - static_cast<double>(k);
  const auto node = [&](int dx, int dy, int dz) {
    return pointInCell(
        cell_bounds,
        static_cast<double>(i + dx) / static_cast<double>(subgrid),
        static_cast<double>(j + dy) / static_cast<double>(subgrid),
        static_cast<double>(k + dz) / static_cast<double>(subgrid));
  };
  const double c000 = exact_phi(node(0, 0, 0));
  const double c100 = exact_phi(node(1, 0, 0));
  const double c010 = exact_phi(node(0, 1, 0));
  const double c110 = exact_phi(node(1, 1, 0));
  const double c001 = exact_phi(node(0, 0, 1));
  const double c101 = exact_phi(node(1, 0, 1));
  const double c011 = exact_phi(node(0, 1, 1));
  const double c111 = exact_phi(node(1, 1, 1));
  const double c00 = lerp(c000, c100, tx);
  const double c10 = lerp(c010, c110, tx);
  const double c01 = lerp(c001, c101, tx);
  const double c11 = lerp(c011, c111, tx);
  const double c0 = lerp(c00, c10, ty);
  const double c1 = lerp(c01, c11, ty);
  result.phi = lerp(c0, c1, tz);
  result.sign = signWithEps(result.phi, sign_epsilon);
  result.fixed_mismatch =
      original_sdf_sign != reference_sign && result.sign == reference_sign;
  result.remaining_mismatch =
      reference_sign != 0 && result.sign != 0 && result.sign != reference_sign;
  return result;
}

}  // namespace adasdf
