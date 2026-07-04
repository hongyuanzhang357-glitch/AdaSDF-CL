#include "adasdf/sampling/ExactBlockSamplerKernel.h"

#include <algorithm>
#include <chrono>

namespace adasdf {
namespace {

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

}  // namespace

AdaptiveSDFBlock ExactBlockSamplerKernel::sample(
    const AABB& block_bounds,
    int block_id,
    int octree_node_id,
    int level,
    int block_resolution,
    BVHSDFSampler& sampler,
    const ExactBlockSamplingKernelOptions& options,
    ExactBlockSamplingKernelStats* stats) {
  const auto begin = std::chrono::steady_clock::now();
  const int resolution = std::max(2, block_resolution);
  AdaptiveSDFBlock block = makeBlockHeader(
      block_bounds,
      block_id,
      octree_node_id,
      level,
      resolution,
      options.signed_distance);
  block.phi.resize(
      static_cast<std::size_t>(resolution) * static_cast<std::size_t>(resolution) *
      static_cast<std::size_t>(resolution));

  const SDFSamplerCounters counters0 =
      options.enable_counters ? sampler.counters() : SDFSamplerCounters{};
  const TriangleMesh* mesh = sampler.mesh();
  const bool use_direct_static_sampling =
      mesh != nullptr && !options.enable_counters && !options.enable_diagnostics;
  const bool use_bvh = use_direct_static_sampling &&
                       sampler.options().acceleration ==
                           SDFSamplingAcceleration::BVH &&
                       sampler.hasBVH();
  for (int k = 0; k < resolution; ++k) {
    for (int j = 0; j < resolution; ++j) {
      for (int i = 0; i < resolution; ++i) {
        const Vector3 point = gridPoint(block_bounds, i, j, k, resolution);
        const BVHSDFSampleResult sample =
            use_direct_static_sampling
                ? (use_bvh ? BVHSDFSampler::sampleWithBVH(
                                 *mesh,
                                 sampler.bvh(),
                                 point,
                                 sampler.options())
                           : BVHSDFSampler::sampleBruteForce(
                                 *mesh,
                                 point,
                                 options.signed_distance))
                : sampler.sample(point);
        block.phi[gridIndex(i, j, k, resolution, resolution)] =
            sample.success ? sample.phi : 0.0;
      }
    }
  }
  if (stats != nullptr) {
    const auto end = std::chrono::steady_clock::now();
    stats->sample_count += block.phi.size();
    stats->time_ms += std::chrono::duration<double, std::milli>(end - begin).count();
    if (options.enable_counters) {
      const SDFSamplerCounters counters1 = sampler.counters();
      stats->distance_query_count +=
          counters1.distance_query_count - counters0.distance_query_count;
      stats->sign_query_count +=
          counters1.sign_query_count - counters0.sign_query_count;
    } else if (options.enable_diagnostics) {
      stats->distance_query_count += block.phi.size();
      if (options.signed_distance) {
        stats->sign_query_count += block.phi.size();
      }
    }
  }
  return block;
}

}  // namespace adasdf
