#include "adasdf/sampling/HierarchicalBlockSampler.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "adasdf/sampling/HierarchicalSDFPredictor.h"

namespace adasdf {
namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

Vector3 gridPoint(const AABB& bounds, int i, int j, int k, int nx, int ny, int nz) {
  const double tx = nx <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(nx - 1);
  const double ty = ny <= 1 ? 0.0 : static_cast<double>(j) / static_cast<double>(ny - 1);
  const double tz = nz <= 1 ? 0.0 : static_cast<double>(k) / static_cast<double>(nz - 1);
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
    bool signed_distance,
    bool near_surface) {
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
  block.near_surface = near_surface;
  block.signed_distance = signed_distance;
  return block;
}

AdaptiveSDFBlock sampleCoarseBlock(
    const AABB& bounds,
    int block_id,
    int octree_node_id,
    int level,
    int resolution,
    bool signed_distance,
    bool near_surface,
    BVHSDFSampler& exact_sampler) {
  AdaptiveSDFBlock block = makeBlockHeader(
      bounds,
      block_id,
      octree_node_id,
      level,
      resolution,
      signed_distance,
      near_surface);
  block.phi.resize(
      static_cast<std::size_t>(resolution) * static_cast<std::size_t>(resolution) *
      static_cast<std::size_t>(resolution));
  for (int k = 0; k < resolution; ++k) {
    for (int j = 0; j < resolution; ++j) {
      for (int i = 0; i < resolution; ++i) {
        const BVHSDFSampleResult sample =
            exact_sampler.sample(gridPoint(bounds, i, j, k, resolution, resolution, resolution));
        block.phi[gridIndex(i, j, k, resolution, resolution)] =
            sample.success ? sample.phi : 0.0;
      }
    }
  }
  return block;
}

SamplingQualityOptions qualityOptionsForDecision(
    const BlockSamplingDecision& decision,
    const HierarchicalSamplingOptions& options) {
  SamplingQualityOptions quality;
  quality.check_samples_per_axis = options.quality_check_samples_per_axis;
  quality.max_abs_error_limit = decision.target_error_for_block;
  quality.rms_error_limit =
      options.target_rms_error *
      (decision.target_error_for_block /
       std::max(1e-30, options.target_max_abs_error));
  quality.p95_error_limit =
      options.target_p95_error *
      (decision.target_error_for_block /
       std::max(1e-30, options.target_max_abs_error));
  quality.near_surface_band = options.near_surface_band;
  return quality;
}

}  // namespace

AdaptiveSDFBlock HierarchicalBlockSampler::sampleBlockExact(
    const AABB& block_bounds,
    int block_id,
    int octree_node_id,
    int level,
    int block_resolution,
    bool signed_distance,
    bool near_surface,
    BVHSDFSampler& exact_sampler) {
  const int resolution = std::max(2, block_resolution);
  AdaptiveSDFBlock block = makeBlockHeader(
      block_bounds,
      block_id,
      octree_node_id,
      level,
      resolution,
      signed_distance,
      near_surface);
  block.phi.resize(
      static_cast<std::size_t>(resolution) * static_cast<std::size_t>(resolution) *
      static_cast<std::size_t>(resolution));
  for (int k = 0; k < resolution; ++k) {
    for (int j = 0; j < resolution; ++j) {
      for (int i = 0; i < resolution; ++i) {
        const BVHSDFSampleResult sample = exact_sampler.sample(
            gridPoint(block_bounds, i, j, k, resolution, resolution, resolution));
        block.phi[gridIndex(i, j, k, resolution, resolution)] =
            sample.success ? sample.phi : 0.0;
      }
    }
  }
  return block;
}

HierarchicalBlockSamplingResult HierarchicalBlockSampler::sampleBlock(
    const AABB& block_bounds,
    int block_id,
    int octree_node_id,
    int level,
    int block_resolution,
    bool signed_distance,
    bool near_surface,
    BVHSDFSampler& exact_sampler,
    const HierarchicalSamplingOptions& options) {
  HierarchicalBlockSamplingResult result;
  const auto total0 = std::chrono::steady_clock::now();
  const int fine_resolution = std::max(2, block_resolution);
  const int coarse_resolution = std::max(2, options.coarse_resolution);

  if (!block_bounds.valid) {
    result.error_message = "invalid block bounds";
    return result;
  }

  const auto coarse0 = std::chrono::steady_clock::now();
  AdaptiveSDFBlock coarse = sampleCoarseBlock(
      block_bounds,
      block_id,
      octree_node_id,
      level,
      coarse_resolution,
      signed_distance,
      near_surface,
      exact_sampler);
  const auto coarse1 = std::chrono::steady_clock::now();
  result.coarse_sample_count = coarse.phi.size();

  BlockClassificationOptions classification_options;
  classification_options.near_surface_band = options.near_surface_band;
  BlockClassificationResult classification =
      BlockClassifier::classify(coarse, classification_options);
  if (near_surface) {
    classification.importance = BlockImportanceClass::NearSurface;
  }
  result.decision =
      HierarchicalSamplingPolicy::decide(block_id, classification, options);

  if (result.decision.mode == BlockSamplingMode::ExactBVH) {
    result.block = sampleBlockExact(
        block_bounds,
        block_id,
        octree_node_id,
        level,
        fine_resolution,
        signed_distance,
        near_surface,
        exact_sampler);
    result.exact_sample_count = result.block.phi.size();
    result.exact_sampling_time_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - coarse0)
            .count();
    result.success = true;
    result.total_time_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - total0)
            .count();
    return result;
  }

  const auto pred0 = std::chrono::steady_clock::now();
  SDFPredictionResult prediction =
      HierarchicalSDFPredictor::predictFromCoarseSamples(
          block_bounds,
          fine_resolution,
          fine_resolution,
          fine_resolution,
          coarse.phi,
          coarse_resolution,
          coarse_resolution,
          coarse_resolution);
  const auto pred1 = std::chrono::steady_clock::now();
  result.prediction_time_ms =
      std::chrono::duration<double, std::milli>(pred1 - pred0).count();
  result.predicted_sample_count = prediction.predicted_sample_count;
  if (!prediction.success) {
    result.block = sampleBlockExact(
        block_bounds,
        block_id,
        octree_node_id,
        level,
        fine_resolution,
        signed_distance,
        near_surface,
        exact_sampler);
    result.exact_sample_count = result.block.phi.size() + result.coarse_sample_count;
    result.fallback_exact = true;
    result.error_message = prediction.error_message;
    result.success = true;
    result.total_time_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - total0)
            .count();
    return result;
  }

  const auto guard0 = std::chrono::steady_clock::now();
  result.quality = SamplingQualityGuard::check(
      block_bounds,
      fine_resolution,
      fine_resolution,
      fine_resolution,
      prediction.predicted_phi,
      exact_sampler,
      qualityOptionsForDecision(result.decision, options));
  const auto guard1 = std::chrono::steady_clock::now();
  result.quality_check_time_ms =
      std::chrono::duration<double, std::milli>(guard1 - guard0).count();

  if (result.quality.accepted) {
    result.block = makeBlockHeader(
        block_bounds,
        block_id,
        octree_node_id,
        level,
        fine_resolution,
        signed_distance,
        near_surface);
    result.block.phi = std::move(prediction.predicted_phi);
    result.used_prediction = true;
    result.exact_sample_count =
        result.coarse_sample_count + result.quality.check_sample_count;
    result.success = true;
  } else if (options.fallback_to_exact_on_quality_fail) {
    const auto exact0 = std::chrono::steady_clock::now();
    result.block = sampleBlockExact(
        block_bounds,
        block_id,
        octree_node_id,
        level,
        fine_resolution,
        signed_distance,
        near_surface,
        exact_sampler);
    const auto exact1 = std::chrono::steady_clock::now();
    result.exact_sampling_time_ms =
        std::chrono::duration<double, std::milli>(exact1 - exact0).count();
    result.exact_sample_count =
        result.coarse_sample_count + result.quality.check_sample_count +
        result.block.phi.size();
    result.fallback_exact = true;
    result.success = true;
  } else {
    result.error_message = "prediction rejected by quality guard";
    result.exact_sample_count =
        result.coarse_sample_count + result.quality.check_sample_count;
    result.success = false;
  }

  result.exact_sampling_time_ms +=
      std::chrono::duration<double, std::milli>(coarse1 - coarse0).count();
  result.total_time_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - total0)
          .count();
  return result;
}

}  // namespace adasdf
