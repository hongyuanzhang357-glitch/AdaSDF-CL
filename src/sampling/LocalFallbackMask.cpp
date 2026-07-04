#include "adasdf/sampling/LocalFallbackMask.h"

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

bool inHalo(int i, int j, int k, int nx, int ny, int nz, int halo_layers) {
  if (halo_layers <= 0) {
    return false;
  }
  return i < halo_layers || j < halo_layers || k < halo_layers ||
         i >= nx - halo_layers || j >= ny - halo_layers ||
         k >= nz - halo_layers;
}

}  // namespace

bool LocalFallbackMask::valid() const {
  return nx > 0 && ny > 0 && nz > 0 &&
         exact_required.size() ==
             static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
                 static_cast<std::size_t>(nz);
}

bool LocalFallbackMask::isExactRequired(int i, int j, int k) const {
  if (!valid() || i < 0 || j < 0 || k < 0 || i >= nx || j >= ny || k >= nz) {
    return false;
  }
  return exact_required[gridIndex(i, j, k, nx, ny)] != 0;
}

std::size_t LocalFallbackMask::exactRequiredCount() const {
  return static_cast<std::size_t>(
      std::count(exact_required.begin(), exact_required.end(), std::uint8_t{1}));
}

LocalFallbackMask LocalFallbackMaskBuilder::build(
    int nx,
    int ny,
    int nz,
    const std::vector<double>& predicted_phi,
    double near_surface_band,
    int halo_exact_layers) {
  LocalFallbackMask mask;
  mask.nx = nx;
  mask.ny = ny;
  mask.nz = nz;
  const std::size_t count =
      static_cast<std::size_t>(std::max(0, nx)) *
      static_cast<std::size_t>(std::max(0, ny)) *
      static_cast<std::size_t>(std::max(0, nz));
  mask.exact_required.assign(count, std::uint8_t{0});
  if (nx <= 0 || ny <= 0 || nz <= 0 || predicted_phi.size() != count) {
    return mask;
  }
  const double band = std::max(0.0, near_surface_band);
  const int halo = std::max(0, halo_exact_layers);
  for (int k = 0; k < nz; ++k) {
    for (int j = 0; j < ny; ++j) {
      for (int i = 0; i < nx; ++i) {
        const std::size_t index = gridIndex(i, j, k, nx, ny);
        const double phi = predicted_phi[index];
        if (inHalo(i, j, k, nx, ny, nz, halo) ||
            !std::isfinite(phi) || std::abs(phi) <= band) {
          mask.exact_required[index] = 1;
        }
      }
    }
  }
  return mask;
}

}  // namespace adasdf
