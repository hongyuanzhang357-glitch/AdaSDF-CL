#include "adasdf/runtime/ExpandedBlock.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double finiteOrZero(double value) {
  return std::isfinite(value) ? value : 0.0;
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

}  // namespace

bool ActiveExpandedBlock::isValid() const {
  if (block_id < 0 || nx < 2 || ny < 2 || nz < 2) {
    return false;
  }
  if (!bounds.valid || !bounds.min.allFinite() || !bounds.max.allFinite() ||
      !origin.allFinite() || !spacing.allFinite()) {
    return false;
  }
  if (!(spacing.x > 0.0) || !(spacing.y > 0.0) || !(spacing.z > 0.0)) {
    return false;
  }
  if (phi.size() != valueCount()) {
    return false;
  }
  return std::all_of(phi.begin(), phi.end(), [](double value) {
    return std::isfinite(value);
  });
}

bool ActiveExpandedBlock::contains(const Vector3& point) const {
  const double eps = 1.0e-12;
  return bounds.valid &&
         point.x >= bounds.min.x - eps && point.x <= bounds.max.x + eps &&
         point.y >= bounds.min.y - eps && point.y <= bounds.max.y + eps &&
         point.z >= bounds.min.z - eps && point.z <= bounds.max.z + eps;
}

std::size_t ActiveExpandedBlock::valueCount() const {
  return static_cast<std::size_t>(std::max(0, nx)) *
         static_cast<std::size_t>(std::max(0, ny)) *
         static_cast<std::size_t>(std::max(0, nz));
}

std::size_t ActiveExpandedBlock::memoryBytes() const {
  return sizeof(ActiveExpandedBlock) + sizeof(double) * phi.size();
}

double ActiveExpandedBlock::sampleDistance(const Vector3& point) const {
  if (!isValid()) {
    return 0.0;
  }

  const double x = axisCoord(point.x, origin.x, spacing.x, nx);
  const double y = axisCoord(point.y, origin.y, spacing.y, ny);
  const double z = axisCoord(point.z, origin.z, spacing.z, nz);

  const int i0 = static_cast<int>(std::floor(x));
  const int j0 = static_cast<int>(std::floor(y));
  const int k0 = static_cast<int>(std::floor(z));
  const int i1 = std::min(i0 + 1, nx - 1);
  const int j1 = std::min(j0 + 1, ny - 1);
  const int k1 = std::min(k0 + 1, nz - 1);
  const double tx = x - static_cast<double>(i0);
  const double ty = y - static_cast<double>(j0);
  const double tz = z - static_cast<double>(k0);

  const auto value = [&](int i, int j, int k) {
    return finiteOrZero(phi[valueIndex(i, j, k, nx, ny)]);
  };

  const double c00 = lerp(value(i0, j0, k0), value(i1, j0, k0), tx);
  const double c10 = lerp(value(i0, j1, k0), value(i1, j1, k0), tx);
  const double c01 = lerp(value(i0, j0, k1), value(i1, j0, k1), tx);
  const double c11 = lerp(value(i0, j1, k1), value(i1, j1, k1), tx);
  const double c0 = lerp(c00, c10, ty);
  const double c1 = lerp(c01, c11, ty);
  return finiteOrZero(lerp(c0, c1, tz));
}

Vector3 ActiveExpandedBlock::sampleGradient(const Vector3& point) const {
  if (!isValid()) {
    return {};
  }
  const double hx = std::max(spacing.x * 0.5, 1.0e-7);
  const double hy = std::max(spacing.y * 0.5, 1.0e-7);
  const double hz = std::max(spacing.z * 0.5, 1.0e-7);

  const Vector3 mx{point.x - hx, point.y, point.z};
  const Vector3 px{point.x + hx, point.y, point.z};
  const Vector3 my{point.x, point.y - hy, point.z};
  const Vector3 py{point.x, point.y + hy, point.z};
  const Vector3 mz{point.x, point.y, point.z - hz};
  const Vector3 pz{point.x, point.y, point.z + hz};

  return {
      finiteOrZero((sampleDistance(px) - sampleDistance(mx)) / (2.0 * hx)),
      finiteOrZero((sampleDistance(py) - sampleDistance(my)) / (2.0 * hy)),
      finiteOrZero((sampleDistance(pz) - sampleDistance(mz)) / (2.0 * hz))};
}

}  // namespace adasdf
