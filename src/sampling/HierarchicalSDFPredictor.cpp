#include "adasdf/sampling/HierarchicalSDFPredictor.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double axisT(double value, double lo, double hi) {
  if (!(hi > lo)) {
    return 0.0;
  }
  return std::min(1.0, std::max(0.0, (value - lo) / (hi - lo)));
}

double interpolateUnitGrid(
    int nx,
    int ny,
    int nz,
    const std::vector<double>& phi,
    double tx,
    double ty,
    double tz) {
  const double gx = tx * static_cast<double>(nx - 1);
  const double gy = ty * static_cast<double>(ny - 1);
  const double gz = tz * static_cast<double>(nz - 1);
  const int i0 = std::min(nx - 1, std::max(0, static_cast<int>(std::floor(gx))));
  const int j0 = std::min(ny - 1, std::max(0, static_cast<int>(std::floor(gy))));
  const int k0 = std::min(nz - 1, std::max(0, static_cast<int>(std::floor(gz))));
  const int i1 = std::min(nx - 1, i0 + 1);
  const int j1 = std::min(ny - 1, j0 + 1);
  const int k1 = std::min(nz - 1, k0 + 1);
  const double fx = gx - static_cast<double>(i0);
  const double fy = gy - static_cast<double>(j0);
  const double fz = gz - static_cast<double>(k0);

  const auto at = [&](int i, int j, int k) {
    return phi[gridIndex(i, j, k, nx, ny)];
  };
  const double c00 = at(i0, j0, k0) * (1.0 - fx) + at(i1, j0, k0) * fx;
  const double c10 = at(i0, j1, k0) * (1.0 - fx) + at(i1, j1, k0) * fx;
  const double c01 = at(i0, j0, k1) * (1.0 - fx) + at(i1, j0, k1) * fx;
  const double c11 = at(i0, j1, k1) * (1.0 - fx) + at(i1, j1, k1) * fx;
  const double c0 = c00 * (1.0 - fy) + c10 * fy;
  const double c1 = c01 * (1.0 - fy) + c11 * fy;
  return c0 * (1.0 - fz) + c1 * fz;
}

}  // namespace

double interpolateGridPhi(
    const AABB& bounds,
    int nx,
    int ny,
    int nz,
    const std::vector<double>& phi,
    const Vector3& point) {
  if (!bounds.valid || nx < 2 || ny < 2 || nz < 2 ||
      phi.size() !=
          static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
              static_cast<std::size_t>(nz)) {
    return 0.0;
  }
  const double tx = axisT(point.x, bounds.min.x, bounds.max.x);
  const double ty = axisT(point.y, bounds.min.y, bounds.max.y);
  const double tz = axisT(point.z, bounds.min.z, bounds.max.z);
  return interpolateUnitGrid(nx, ny, nz, phi, tx, ty, tz);
}

SDFPredictionResult HierarchicalSDFPredictor::predictFromCoarseSamples(
    const AABB& block_bounds,
    int nx,
    int ny,
    int nz,
    const std::vector<double>& coarse_phi,
    int coarse_nx,
    int coarse_ny,
    int coarse_nz) {
  SDFPredictionResult result;
  result.nx = nx;
  result.ny = ny;
  result.nz = nz;
  if (!block_bounds.valid) {
    result.error_message = "invalid block bounds";
    return result;
  }
  if (nx < 2 || ny < 2 || nz < 2 || coarse_nx < 2 || coarse_ny < 2 ||
      coarse_nz < 2) {
    result.error_message = "fine and coarse resolutions must be at least 2";
    return result;
  }
  const std::size_t coarse_count =
      static_cast<std::size_t>(coarse_nx) * static_cast<std::size_t>(coarse_ny) *
      static_cast<std::size_t>(coarse_nz);
  if (coarse_phi.size() != coarse_count) {
    result.error_message = "coarse phi size does not match coarse grid";
    return result;
  }
  for (const double value : coarse_phi) {
    if (!std::isfinite(value)) {
      result.error_message = "coarse phi contains non-finite values";
      return result;
    }
  }

  result.predicted_phi.resize(
      static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
      static_cast<std::size_t>(nz));
  for (int k = 0; k < nz; ++k) {
    const double tz = static_cast<double>(k) / static_cast<double>(nz - 1);
    for (int j = 0; j < ny; ++j) {
      const double ty = static_cast<double>(j) / static_cast<double>(ny - 1);
      for (int i = 0; i < nx; ++i) {
        const double tx = static_cast<double>(i) / static_cast<double>(nx - 1);
        const double value = interpolateUnitGrid(
            coarse_nx,
            coarse_ny,
            coarse_nz,
            coarse_phi,
            tx,
            ty,
            tz);
        if (!std::isfinite(value)) {
          result.error_message = "prediction produced non-finite values";
          return result;
        }
        result.predicted_phi[gridIndex(i, j, k, nx, ny)] = value;
      }
    }
  }
  result.exact_sample_count = coarse_count;
  result.predicted_sample_count = result.predicted_phi.size();
  result.success = true;
  return result;
}

}  // namespace adasdf
