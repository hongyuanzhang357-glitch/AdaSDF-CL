#include "adasdf/narrowband/BrickTensorGenerator.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

#include "adasdf/narrowband/BrickTensorFillPolicy.h"

namespace adasdf {
namespace {

Vector3 gridPoint(const AABB& box, int ix, int iy, int iz, int nx, int ny, int nz) {
  const double x = nx > 1 ? static_cast<double>(ix) / static_cast<double>(nx - 1) : 0.0;
  const double y = ny > 1 ? static_cast<double>(iy) / static_cast<double>(ny - 1) : 0.0;
  const double z = nz > 1 ? static_cast<double>(iz) / static_cast<double>(nz - 1) : 0.0;
  return {
      box.min.x + (box.max.x - box.min.x) * x,
      box.min.y + (box.max.y - box.min.y) * y,
      box.min.z + (box.max.z - box.min.z) * z};
}

std::size_t valueIndex(int ix, int iy, int iz, int nx, int ny) {
  return static_cast<std::size_t>(ix) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(iy) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(iz));
}

int signOf(double value) {
  if (value > 0.0) {
    return 1;
  }
  if (value < 0.0) {
    return -1;
  }
  return 0;
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double cellCenterInterpolatedPhi(
    const std::vector<double>& phi,
    int ix,
    int iy,
    int iz,
    int nx,
    int ny) {
  const int i1 = ix + 1;
  const int j1 = iy + 1;
  const int k1 = iz + 1;
  const double c00 =
      lerp(phi[valueIndex(ix, iy, iz, nx, ny)],
           phi[valueIndex(i1, iy, iz, nx, ny)],
           0.5);
  const double c10 =
      lerp(phi[valueIndex(ix, j1, iz, nx, ny)],
           phi[valueIndex(i1, j1, iz, nx, ny)],
           0.5);
  const double c01 =
      lerp(phi[valueIndex(ix, iy, k1, nx, ny)],
           phi[valueIndex(i1, iy, k1, nx, ny)],
           0.5);
  const double c11 =
      lerp(phi[valueIndex(ix, j1, k1, nx, ny)],
           phi[valueIndex(i1, j1, k1, nx, ny)],
           0.5);
  return lerp(lerp(c00, c10, 0.5), lerp(c01, c11, 0.5), 0.5);
}

}  // namespace

BrickTensorResult BrickTensorGenerator::generate(
    const CompressionBrick& brick,
    const BVHSDFSampler& sampler,
    const NarrowBandBrickBuildOptions& options) {
  BrickTensorResult result;
  result.brick = brick;
  AdaptiveSDFBlock block;
  block.block_id = brick.brick_id;
  block.octree_node_id = brick.brick_id;
  block.level = brick.brick_level;
  block.bounds = brick.bounds;
  block.nx = brick.tensor_nx;
  block.ny = brick.tensor_ny;
  block.nz = brick.tensor_nz;
  block.origin = brick.bounds.min;
  block.spacing = {
      (brick.bounds.max.x - brick.bounds.min.x) /
          static_cast<double>(std::max(1, block.nx - 1)),
      (brick.bounds.max.y - brick.bounds.min.y) /
          static_cast<double>(std::max(1, block.ny - 1)),
      (brick.bounds.max.z - brick.bounds.min.z) /
          static_cast<double>(std::max(1, block.nz - 1))};
  block.near_surface = brick.contact_band_node_count > 0;
  block.signed_distance = true;
  const std::size_t count =
      static_cast<std::size_t>(block.nx) * static_cast<std::size_t>(block.ny) *
      static_cast<std::size_t>(block.nz);
  block.phi.assign(count, 0.0);
  std::vector<unsigned char> exact(count, 0);
  std::size_t exact_count = 0;
  auto markExact = [&](int ix, int iy, int iz, bool fallback_exact) {
    if (ix < 0 || iy < 0 || iz < 0 || ix >= block.nx || iy >= block.ny ||
        iz >= block.nz) {
      return;
    }
    const std::size_t idx = valueIndex(ix, iy, iz, block.nx, block.ny);
    if (exact[idx] == 0) {
      exact[idx] = 1;
      ++exact_count;
      if (fallback_exact) {
        ++result.brick.fill_fallback_exact_node_count;
      }
    }
  };

  const double exact_threshold = std::max(
      options.sampling_contact_band_width,
      options.contact_exact_spacing > 0.0
          ? options.contact_exact_spacing
          : options.sampling_contact_band_width);
  for (int iz = 0; iz < block.nz; ++iz) {
    for (int iy = 0; iy < block.ny; ++iy) {
      for (int ix = 0; ix < block.nx; ++ix) {
        const Vector3 p =
            gridPoint(block.bounds, ix, iy, iz, block.nx, block.ny, block.nz);
        const BrickTensorFillDecision fill =
            BrickTensorFillPolicy::evaluate(p, sampler, options);
        const std::size_t idx = valueIndex(ix, iy, iz, block.nx, block.ny);
        block.phi[idx] = fill.phi;
        if (fill.exact_source || std::abs(fill.phi) <= exact_threshold) {
          markExact(ix, iy, iz, false);
        }
      }
    }
  }

  const bool protect_zero_crossings =
      options.contact_exact_from_zero_crossing_cells ||
      options.sign_protected_fill || options.fill_sign_check;
  const int stencil = std::max(
      1,
      std::max(
          options.contact_exact_stencil,
          options.zero_crossing_exact_stencil));
  for (int iz = 0; iz + 1 < block.nz; ++iz) {
    for (int iy = 0; iy + 1 < block.ny; ++iy) {
      for (int ix = 0; ix + 1 < block.nx; ++ix) {
        bool has_pos = false;
        bool has_neg = false;
        for (int dz = 0; dz <= 1; ++dz) {
          for (int dy = 0; dy <= 1; ++dy) {
            for (int dx = 0; dx <= 1; ++dx) {
              const int sign = signOf(block.phi[valueIndex(
                  ix + dx,
                  iy + dy,
                  iz + dz,
                  block.nx,
                  block.ny)]);
              has_pos = has_pos || sign > 0;
              has_neg = has_neg || sign < 0;
            }
          }
        }
        const bool zero_crossing = has_pos && has_neg;
        if (zero_crossing) {
          ++result.brick.zero_crossing_cell_count;
        }

        bool sign_mismatch = false;
        if (options.fill_sign_check) {
          ++result.brick.sign_check_node_count;
          const Vector3 center = gridPoint(
              block.bounds,
              ix * 2 + 1,
              iy * 2 + 1,
              iz * 2 + 1,
              (block.nx - 1) * 2 + 1,
              (block.ny - 1) * 2 + 1,
              (block.nz - 1) * 2 + 1);
          const BVHSDFSampleResult exact_center = sampler.sample(center);
          const double interp_phi = cellCenterInterpolatedPhi(
              block.phi,
              ix,
              iy,
              iz,
              block.nx,
              block.ny);
          if (exact_center.success && std::isfinite(exact_center.phi) &&
              signOf(exact_center.phi) != signOf(interp_phi)) {
            sign_mismatch = true;
            ++result.brick.sign_check_mismatch_count;
          }
        }

        if (!protect_zero_crossings && !sign_mismatch) {
          continue;
        }
        if (!zero_crossing && !sign_mismatch) {
          continue;
        }
        ++result.brick.protected_zero_crossing_cell_count;
        for (int dz = -stencil + 1; dz <= stencil; ++dz) {
          for (int dy = -stencil + 1; dy <= stencil; ++dy) {
            for (int dx = -stencil + 1; dx <= stencil; ++dx) {
              markExact(ix + dx, iy + dy, iz + dz, true);
            }
          }
        }
      }
    }
  }

  const std::size_t min_exact_nodes = static_cast<std::size_t>(
      std::ceil(std::clamp(options.contact_exact_min_node_ratio, 0.0, 1.0) *
                static_cast<double>(count)));
  if (exact_count < min_exact_nodes) {
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(
        order.begin(),
        order.end(),
        [&](std::size_t a, std::size_t b) {
          return std::abs(block.phi[a]) < std::abs(block.phi[b]);
        });
    for (std::size_t n = 0; n < order.size() &&
                            exact_count < min_exact_nodes; ++n) {
      const std::size_t idx = order[n];
      if (exact[idx] == 0) {
        exact[idx] = 1;
        ++exact_count;
        ++result.brick.fill_fallback_exact_node_count;
      }
    }
  }

  result.brick.exact_source_node_count = exact_count;
  result.brick.interpolated_fill_node_count = count - exact_count;
  result.brick.tensor_node_count = count;
  result.brick.estimated_expanded_bytes = count * sizeof(double);
  result.brick.estimated_compressed_bytes = result.brick.estimated_expanded_bytes;
  result.block = std::move(block);
  return result;
}

}  // namespace adasdf
