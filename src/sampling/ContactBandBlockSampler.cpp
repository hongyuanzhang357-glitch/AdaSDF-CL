#include "adasdf/sampling/ContactBandBlockSampler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

namespace adasdf {
namespace {

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

Vector3 gridPoint(const AABB& bounds, int i, int j, int k, int n) {
  const double tx = n <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
  const double ty = n <= 1 ? 0.0 : static_cast<double>(j) / static_cast<double>(n - 1);
  const double tz = n <= 1 ? 0.0 : static_cast<double>(k) / static_cast<double>(n - 1);
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * tx,
      bounds.min.y + (bounds.max.y - bounds.min.y) * ty,
      bounds.min.z + (bounds.max.z - bounds.min.z) * tz};
}

AdaptiveSDFBlock makeBlockHeader(
    const AABB& bounds,
    int block_id,
    int octree_node_id,
    int level,
    int resolution,
    bool signed_distance) {
  AdaptiveSDFBlock block;
  block.block_id = block_id;
  block.octree_node_id = octree_node_id;
  block.level = level;
  block.bounds = bounds;
  block.nx = resolution;
  block.ny = resolution;
  block.nz = resolution;
  block.origin = bounds.min;
  block.spacing = {
      resolution <= 1 ? 0.0 : (bounds.max.x - bounds.min.x) /
                                 static_cast<double>(resolution - 1),
      resolution <= 1 ? 0.0 : (bounds.max.y - bounds.min.y) /
                                 static_cast<double>(resolution - 1),
      resolution <= 1 ? 0.0 : (bounds.max.z - bounds.min.z) /
                                 static_cast<double>(resolution - 1)};
  block.signed_distance = signed_distance;
  return block;
}

BVHSDFSampleResult sampleExactPoint(
    BVHSDFSampler& sampler,
    const Vector3& point,
    bool signed_distance) {
  const TriangleMesh* mesh = sampler.mesh();
  if (mesh != nullptr &&
      sampler.options().acceleration == SDFSamplingAcceleration::BVH &&
      sampler.hasBVH() &&
      !sampler.options().enable_counters) {
    return BVHSDFSampler::sampleWithBVH(
        *mesh,
        sampler.bvh(),
        point,
        sampler.options());
  }
  if (mesh != nullptr && !sampler.options().enable_counters) {
    return BVHSDFSampler::sampleBruteForce(*mesh, point, signed_distance);
  }
  return sampler.sample(point);
}

double trilerp(
    const std::vector<double>& values,
    int n,
    double x,
    double y,
    double z) {
  const auto clamp01 = [](double value) {
    return std::max(0.0, std::min(1.0, value));
  };
  x = clamp01(x);
  y = clamp01(y);
  z = clamp01(z);
  const double gx = x * static_cast<double>(n - 1);
  const double gy = y * static_cast<double>(n - 1);
  const double gz = z * static_cast<double>(n - 1);
  const int x0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gx))));
  const int y0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gy))));
  const int z0 = std::max(0, std::min(n - 1, static_cast<int>(std::floor(gz))));
  const int x1 = std::min(n - 1, x0 + 1);
  const int y1 = std::min(n - 1, y0 + 1);
  const int z1 = std::min(n - 1, z0 + 1);
  const double tx = gx - static_cast<double>(x0);
  const double ty = gy - static_cast<double>(y0);
  const double tz = gz - static_cast<double>(z0);
  const auto at = [&](int i, int j, int k) {
    return values[gridIndex(i, j, k, n, n)];
  };
  const double c00 = at(x0, y0, z0) * (1.0 - tx) + at(x1, y0, z0) * tx;
  const double c10 = at(x0, y1, z0) * (1.0 - tx) + at(x1, y1, z0) * tx;
  const double c01 = at(x0, y0, z1) * (1.0 - tx) + at(x1, y0, z1) * tx;
  const double c11 = at(x0, y1, z1) * (1.0 - tx) + at(x1, y1, z1) * tx;
  const double c0 = c00 * (1.0 - ty) + c10 * ty;
  const double c1 = c01 * (1.0 - ty) + c11 * ty;
  return c0 * (1.0 - tz) + c1 * tz;
}

double applyFarFieldMode(
    double value,
    double reference_sign_value,
    const ContactBandSamplingOptions& options) {
  if (options.far_field_mode == ContactBandFarFieldMode::CoarseInterpolate) {
    return value;
  }
  const double sign = reference_sign_value < 0.0 ? -1.0 : 1.0;
  if (options.far_field_mode == ContactBandFarFieldMode::ConstantSign) {
    return sign * std::abs(value);
  }
  const double width = std::max(options.contact_band_width, 1e-12);
  return sign * std::max(width, std::abs(value));
}

bool shouldExactNode(
    const ContactBandMask& mask,
    std::size_t index,
    bool has_contact_band,
    const ContactBandSamplingOptions& options) {
  if (!has_contact_band || index >= mask.exact_required.size() ||
      mask.exact_required[index] == 0) {
    return false;
  }
  if (mask.contact_band_node[index] != 0) {
    return options.exact_contact_band_nodes;
  }
  if (mask.halo_node[index] != 0) {
    return options.exact_halo_nodes;
  }
  return options.exact_contact_band_nodes;
}

}  // namespace

ContactBandBlockSamplingResult ContactBandBlockSampler::sampleBlock(
    const AABB& block_bounds,
    int block_id,
    int octree_node_id,
    int level,
    int block_resolution,
    bool signed_distance,
    BVHSDFSampler& exact_sampler,
    const TriangleBVH& triangle_bvh,
    const ContactBandSamplingOptions& options) {
  const auto total0 = std::chrono::steady_clock::now();
  ContactBandBlockSamplingResult result;
  const int n = std::max(2, block_resolution);
  result.block = makeBlockHeader(
      block_bounds,
      block_id,
      octree_node_id,
      level,
      n,
      signed_distance);
  result.block.phi.assign(
      static_cast<std::size_t>(n) * static_cast<std::size_t>(n) *
          static_cast<std::size_t>(n),
      0.0);
  result.mask = ContactBandMarker::markBlock(
      block_bounds,
      n,
      triangle_bvh,
      options.markerOptions());
  result.has_contact_band = result.mask.contact_band_node_count > 0;

  const int coarse_n = std::max(2, options.far_field_resolution);
  std::vector<double> coarse(
      static_cast<std::size_t>(coarse_n) * static_cast<std::size_t>(coarse_n) *
          static_cast<std::size_t>(coarse_n),
      0.0);
  const auto coarse0 = std::chrono::steady_clock::now();
  for (int k = 0; k < coarse_n; ++k) {
    for (int j = 0; j < coarse_n; ++j) {
      for (int i = 0; i < coarse_n; ++i) {
        const BVHSDFSampleResult sample =
            sampleExactPoint(
                exact_sampler,
                gridPoint(block_bounds, i, j, k, coarse_n),
                signed_distance);
        coarse[gridIndex(i, j, k, coarse_n, coarse_n)] =
            sample.success ? sample.phi : 0.0;
        ++result.coarse_sample_count;
      }
    }
  }
  const auto coarse1 = std::chrono::steady_clock::now();
  result.coarse_sampling_time_ms = elapsedMs(coarse0, coarse1);

  const double reference_sign_value = coarse.empty() ? 1.0 : coarse[coarse.size() / 2];
  const auto interp0 = std::chrono::steady_clock::now();
  for (int k = 0; k < n; ++k) {
    const double z = n <= 1 ? 0.0 : static_cast<double>(k) / static_cast<double>(n - 1);
    for (int j = 0; j < n; ++j) {
      const double y = n <= 1 ? 0.0 : static_cast<double>(j) / static_cast<double>(n - 1);
      for (int i = 0; i < n; ++i) {
        const double x = n <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
        const std::size_t index = gridIndex(i, j, k, n, n);
        result.block.phi[index] =
            applyFarFieldMode(
                trilerp(coarse, coarse_n, x, y, z),
                reference_sign_value,
                options);
      }
    }
  }
  const auto interp1 = std::chrono::steady_clock::now();
  result.interpolation_time_ms = elapsedMs(interp0, interp1);

  const auto exact0 = std::chrono::steady_clock::now();
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        const std::size_t index = gridIndex(i, j, k, n, n);
        if (!shouldExactNode(result.mask, index, result.has_contact_band, options)) {
          continue;
        }
        const BVHSDFSampleResult sample =
            sampleExactPoint(
                exact_sampler,
                gridPoint(block_bounds, i, j, k, n),
                signed_distance);
        result.block.phi[index] = sample.success ? sample.phi : result.block.phi[index];
        ++result.exact_node_count;
      }
    }
  }
  const auto exact1 = std::chrono::steady_clock::now();
  result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
  result.predicted_node_count = result.block.phi.size() - result.exact_node_count;
  result.far_field_node_count =
      result.has_contact_band ? result.predicted_node_count : result.block.phi.size();

  const auto total1 = std::chrono::steady_clock::now();
  result.total_time_ms = elapsedMs(total0, total1);
  result.success = true;
  result.diagnostics.total_block_count = 1;
  result.diagnostics.contact_band_block_count = result.has_contact_band ? 1 : 0;
  result.diagnostics.far_field_block_count = result.has_contact_band ? 0 : 1;
  result.diagnostics.total_node_count = result.block.phi.size();
  result.diagnostics.exact_node_count = result.exact_node_count;
  result.diagnostics.predicted_node_count = result.predicted_node_count;
  result.diagnostics.far_field_node_count = result.far_field_node_count;
  result.diagnostics.coarse_sample_count = result.coarse_sample_count;
  result.diagnostics.distance_query_count =
      result.exact_node_count + result.coarse_sample_count;
  result.diagnostics.sign_query_count =
      signed_distance ? result.diagnostics.distance_query_count : 0;
  result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
  result.diagnostics.coarse_sampling_time_ms = result.coarse_sampling_time_ms;
  result.diagnostics.interpolation_time_ms = result.interpolation_time_ms;
  result.diagnostics.total_time_ms = result.total_time_ms;
  finalizeContactBandDiagnostics(&result.diagnostics);
  return result;
}

}  // namespace adasdf
