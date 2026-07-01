#include "adasdf/mesh/TriangleDistance.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

double squaredNorm(const Vector3& v) {
  return dot(v, v);
}

double pointPointSquaredDistance(const Vector3& p, const Vector3& a) {
  return squaredNorm(p - a);
}

double pointSegmentSquaredDistance(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b) {
  const Vector3 ab = b - a;
  const double denom = squaredNorm(ab);
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    return pointPointSquaredDistance(p, a);
  }
  const double t = std::clamp(dot(p - a, ab) / denom, 0.0, 1.0);
  const Vector3 closest = a + t * ab;
  return pointPointSquaredDistance(p, closest);
}

bool finite(const Vector3& v) {
  return v.allFinite();
}

}  // namespace

double pointTriangleSquaredDistance(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c) {
  if (!finite(p) || !finite(a) || !finite(b) || !finite(c)) {
    return std::numeric_limits<double>::infinity();
  }

  const Vector3 ab = b - a;
  const Vector3 ac = c - a;
  const Vector3 ap = p - a;
  const double triangle_area2 = squaredNorm(Vector3{
      ab.y * ac.z - ab.z * ac.y,
      ab.z * ac.x - ab.x * ac.z,
      ab.x * ac.y - ab.y * ac.x});
  if (!(triangle_area2 > 1.0e-30) || !std::isfinite(triangle_area2)) {
    return std::min({
        pointSegmentSquaredDistance(p, a, b),
        pointSegmentSquaredDistance(p, b, c),
        pointSegmentSquaredDistance(p, c, a)});
  }

  const double d1 = dot(ab, ap);
  const double d2 = dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) {
    return pointPointSquaredDistance(p, a);
  }

  const Vector3 bp = p - b;
  const double d3 = dot(ab, bp);
  const double d4 = dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3) {
    return pointPointSquaredDistance(p, b);
  }

  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const double v = d1 / (d1 - d3);
    return pointPointSquaredDistance(p, a + v * ab);
  }

  const Vector3 cp = p - c;
  const double d5 = dot(ab, cp);
  const double d6 = dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6) {
    return pointPointSquaredDistance(p, c);
  }

  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
    const double w = d2 / (d2 - d6);
    return pointPointSquaredDistance(p, a + w * ac);
  }

  const double va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
    const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return pointPointSquaredDistance(p, b + w * (c - b));
  }

  const double denom = va + vb + vc;
  if (!(std::abs(denom) > 0.0) || !std::isfinite(denom)) {
    return std::min({
        pointSegmentSquaredDistance(p, a, b),
        pointSegmentSquaredDistance(p, b, c),
        pointSegmentSquaredDistance(p, c, a)});
  }
  const double v = vb / denom;
  const double w = vc / denom;
  const Vector3 closest = a + ab * v + ac * w;
  const double dist2 = pointPointSquaredDistance(p, closest);
  return std::isfinite(dist2) ? dist2 : std::numeric_limits<double>::infinity();
}

double pointTriangleDistance(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c) {
  const double dist2 = pointTriangleSquaredDistance(p, a, b, c);
  if (!std::isfinite(dist2)) {
    return std::numeric_limits<double>::infinity();
  }
  return std::sqrt(std::max(0.0, dist2));
}

}  // namespace adasdf
