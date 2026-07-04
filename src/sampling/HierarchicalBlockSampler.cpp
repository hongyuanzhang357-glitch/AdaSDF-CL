#include "adasdf/sampling/HierarchicalBlockSampler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

#include "adasdf/sampling/HierarchicalSDFPredictor.h"

namespace adasdf {
namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

double diagonalLength(const AABB& bounds) {
  if (!bounds.valid) {
    return 0.0;
  }
  const Vector3 d = bounds.max - bounds.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

std::size_t denseSampleCount(int resolution) {
  const int n = std::max(2, resolution);
  return static_cast<std::size_t>(n) * static_cast<std::size_t>(n) *
         static_cast<std::size_t>(n);
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
  if (decision.importance == BlockImportanceClass::FarField) {
    switch (options.far_field_quality_check) {
      case FarFieldQualityCheckMode::None:
        quality.check_samples_per_axis = 0;
        break;
      case FarFieldQualityCheckMode::Corners:
        quality.check_samples_per_axis = 2;
        break;
      case FarFieldQualityCheckMode::Sparse:
        quality.check_samples_per_axis =
            std::max(2, options.transition_quality_check_samples_per_axis);
        break;
      case FarFieldQualityCheckMode::Full:
        quality.check_samples_per_axis = options.quality_check_samples_per_axis;
        break;
    }
  } else if (decision.importance == BlockImportanceClass::Transition) {
    quality.check_samples_per_axis =
        options.transition_quality_check_samples_per_axis;
  } else {
    quality.check_samples_per_axis = options.quality_check_samples_per_axis;
  }
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

void setClassDiagnostics(
    const BlockSamplingDecision& decision,
    HierarchicalSamplingDiagnostics* diagnostics) {
  if (diagnostics == nullptr) {
    return;
  }
  diagnostics->total_block_count = 1;
  switch (decision.importance) {
    case BlockImportanceClass::NearSurface:
      diagnostics->near_surface_block_count = 1;
      break;
    case BlockImportanceClass::Transition:
      diagnostics->transition_block_count = 1;
      break;
    case BlockImportanceClass::FarField:
      diagnostics->far_field_block_count = 1;
      break;
  }
  if (decision.mode == BlockSamplingMode::ExactBVH) {
    diagnostics->exact_block_count = 1;
  } else {
    diagnostics->predicted_block_count = 1;
  }
}

void copySamplerCounters(
    const BVHSDFSampler& sampler,
    HierarchicalSamplingDiagnostics* diagnostics) {
  if (diagnostics == nullptr) {
    return;
  }
  const SDFSamplerCounters counters = sampler.counters();
  diagnostics->distance_query_count = counters.distance_query_count;
  diagnostics->sign_query_count = counters.sign_query_count;
  diagnostics->triangle_distance_test_count =
      counters.triangle_distance_test_count;
  diagnostics->bvh_node_visit_count = counters.bvh_node_visit_count;
}

bool canSkipFarFieldQualityCheck(
    const BlockClassificationResult& classification,
    const AABB& bounds,
    const HierarchicalSamplingOptions& options) {
  if (options.far_field_quality_check != FarFieldQualityCheckMode::None) {
    return false;
  }
  const double diag = diagonalLength(bounds);
  if (diag <= 0.0 || !std::isfinite(classification.min_abs_phi)) {
    return false;
  }
  return classification.importance == BlockImportanceClass::FarField &&
         classification.min_abs_phi >
             std::max(0.0, options.far_field_safety_factor) * diag;
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
  if (options.hierarchical_diagnostics) {
    exact_sampler.resetCounters();
  }
  result.diagnostics.fine_sample_count = denseSampleCount(fine_resolution);

  if (!block_bounds.valid) {
    result.error_message = "invalid block bounds";
    return result;
  }

  if (near_surface && options.keep_near_surface_exact) {
    const auto class0 = std::chrono::steady_clock::now();
    BlockClassificationResult classification;
    classification.importance = BlockImportanceClass::NearSurface;
    classification.min_abs_phi = 0.0;
    result.decision =
        HierarchicalSamplingPolicy::decide(block_id, classification, options);
    const auto class1 = std::chrono::steady_clock::now();
    result.diagnostics.classification_time_ms = elapsedMs(class0, class1);
    setClassDiagnostics(result.decision, &result.diagnostics);

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
    result.exact_sample_count = result.block.phi.size();
    result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
    result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
    result.success = true;
    result.total_time_ms =
        elapsedMs(total0, std::chrono::steady_clock::now());
    result.diagnostics.total_hierarchical_time_ms = result.total_time_ms;
    copySamplerCounters(exact_sampler, &result.diagnostics);
    finalizeHierarchicalSamplingDiagnostics(&result.diagnostics);
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
  result.diagnostics.coarse_sample_count = result.coarse_sample_count;
  result.diagnostics.coarse_sampling_time_ms = elapsedMs(coarse0, coarse1);

  const auto class0 = std::chrono::steady_clock::now();
  BlockClassificationOptions classification_options;
  classification_options.near_surface_band = options.near_surface_band;
  BlockClassificationResult classification =
      BlockClassifier::classify(coarse, classification_options);
  if (near_surface) {
    classification.importance = BlockImportanceClass::NearSurface;
  }
  result.decision =
      HierarchicalSamplingPolicy::decide(block_id, classification, options);
  const auto class1 = std::chrono::steady_clock::now();
  result.diagnostics.classification_time_ms = elapsedMs(class0, class1);
  setClassDiagnostics(result.decision, &result.diagnostics);

  if (result.decision.mode == BlockSamplingMode::ExactBVH) {
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
    result.exact_sample_count = result.block.phi.size();
    result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
    result.diagnostics.exact_bvh_sample_count =
        result.coarse_sample_count + result.exact_sample_count;
    result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
    result.success = true;
    result.total_time_ms = elapsedMs(total0, std::chrono::steady_clock::now());
    result.diagnostics.total_hierarchical_time_ms = result.total_time_ms;
    copySamplerCounters(exact_sampler, &result.diagnostics);
    finalizeHierarchicalSamplingDiagnostics(&result.diagnostics);
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
      elapsedMs(pred0, pred1);
  result.predicted_sample_count = prediction.predicted_sample_count;
  result.reused_coarse_sample_count = result.coarse_sample_count;
  result.diagnostics.prediction_time_ms = result.prediction_time_ms;
  result.diagnostics.predicted_sample_count = result.predicted_sample_count;
  result.diagnostics.reused_coarse_sample_count =
      result.reused_coarse_sample_count;
  if (!prediction.success) {
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
    result.exact_sample_count = result.block.phi.size() + result.coarse_sample_count;
    result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
    result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
    result.diagnostics.fallback_exact_block_count = 1;
    result.diagnostics.fallback_due_to_invalid_prediction_count = 1;
    result.diagnostics.fallback_exact_time_ms = result.exact_sampling_time_ms;
    result.fallback_exact = true;
    result.error_message = prediction.error_message;
    result.success = true;
    result.total_time_ms =
        elapsedMs(total0, std::chrono::steady_clock::now());
    result.diagnostics.total_hierarchical_time_ms = result.total_time_ms;
    copySamplerCounters(exact_sampler, &result.diagnostics);
    finalizeHierarchicalSamplingDiagnostics(&result.diagnostics);
    return result;
  }

  if (canSkipFarFieldQualityCheck(classification, block_bounds, options)) {
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
    result.exact_sample_count = result.coarse_sample_count;
    result.diagnostics.accepted_prediction_block_count = 1;
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
    result.diagnostics.skipped_far_field_quality_check_count = 1;
    result.success = true;
    result.total_time_ms = elapsedMs(total0, std::chrono::steady_clock::now());
    result.diagnostics.total_hierarchical_time_ms = result.total_time_ms;
    copySamplerCounters(exact_sampler, &result.diagnostics);
    finalizeHierarchicalSamplingDiagnostics(&result.diagnostics);
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
  result.quality_check_time_ms = elapsedMs(guard0, guard1);
  result.quality_check_sample_count = result.quality.check_sample_count;
  result.diagnostics.quality_check_time_ms = result.quality_check_time_ms;
  result.diagnostics.quality_check_sample_count =
      result.quality_check_sample_count;

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
    result.diagnostics.accepted_prediction_block_count = 1;
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
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
    result.exact_sampling_time_ms = elapsedMs(exact0, exact1);
    result.exact_sample_count =
        result.coarse_sample_count + result.quality.check_sample_count +
        result.block.phi.size();
    result.diagnostics.fallback_exact_block_count = 1;
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
    result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
    result.diagnostics.fallback_exact_time_ms = result.exact_sampling_time_ms;
    if (result.quality.sign_mismatch_count > 0 ||
        result.quality.near_surface_sign_mismatch_count > 0) {
      result.diagnostics.fallback_due_to_sign_count = 1;
    } else {
      result.diagnostics.fallback_due_to_error_count = 1;
    }
    result.fallback_exact = true;
    result.success = true;
  } else {
    result.error_message = "prediction rejected by quality guard";
    result.exact_sample_count =
        result.coarse_sample_count + result.quality.check_sample_count;
    result.diagnostics.fallback_due_to_error_count = 1;
    result.diagnostics.exact_bvh_sample_count = result.exact_sample_count;
    result.success = false;
  }

  result.diagnostics.exact_sampling_time_ms = result.exact_sampling_time_ms;
  result.total_time_ms = elapsedMs(total0, std::chrono::steady_clock::now());
  result.diagnostics.total_hierarchical_time_ms = result.total_time_ms;
  copySamplerCounters(exact_sampler, &result.diagnostics);
  finalizeHierarchicalSamplingDiagnostics(&result.diagnostics);
  return result;
}

}  // namespace adasdf
