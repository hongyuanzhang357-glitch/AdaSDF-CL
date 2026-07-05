#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/acceleration/ParallelSampling.h"
#include "adasdf/generation/AdaptiveBlockPartitioner.h"
#include "adasdf/mesh/MeshCleanup.h"
#include "adasdf/mesh/STLReader.h"
#include "adasdf/sampling/ContactBandBlockSampler.h"
#include "adasdf/sampling/HierarchicalBlockSampler.h"

namespace adasdf {
namespace {

Vector3 gridPoint(const AdaptiveSDFBlock& block, int i, int j, int k) {
  return {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
}

Vector3 gridPointFromIndex(const AdaptiveSDFBlock& block, std::size_t index) {
  const int i = static_cast<int>(index % static_cast<std::size_t>(block.nx));
  const int j = static_cast<int>(
      (index / static_cast<std::size_t>(block.nx)) %
      static_cast<std::size_t>(block.ny));
  const int k = static_cast<int>(
      index /
      (static_cast<std::size_t>(block.nx) *
       static_cast<std::size_t>(block.ny)));
  return gridPoint(block, i, j, k);
}

struct BlockSampleLocation {
  std::size_t block_index = 0;
  std::size_t local_index = 0;
};

struct BulkExactSamplingResult {
  std::size_t sample_count = 0;
  double time_ms = 0.0;
  int threads_used = 1;
};

void sampleBlocks(
    const TriangleMesh& mesh,
    AdaptiveSDFBlockSet& block_set,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport& report) {
  std::vector<BlockSampleLocation> locations;
  for (std::size_t block_index = 0; block_index < block_set.blocks.size();
       ++block_index) {
    AdaptiveSDFBlock& block = block_set.blocks[block_index];
    block.signed_distance = options.signed_distance;
    block.phi.resize(
        static_cast<std::size_t>(block.nx) *
        static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz));
    for (std::size_t local = 0; local < block.phi.size(); ++local) {
      locations.push_back({block_index, local});
    }
  }

  BuildAccelerationStats stats;
  stats.acceleration = options.acceleration;
  stats.threads_requested = std::max(1, options.threads);

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = options.acceleration;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.enable_counters =
      options.hierarchical_sampling.hierarchical_diagnostics;
  sampler_options.bvh_options.degenerate_area_epsilon =
      options.degenerate_area_epsilon;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options, &stats);
  const bool use_bvh =
      options.acceleration == SDFSamplingAcceleration::BVH && sampler.hasBVH();

  std::atomic<std::size_t> nearest_node_visits{0};
  std::atomic<std::size_t> nearest_triangle_tests{0};
  std::atomic<std::size_t> ray_node_visits{0};
  std::atomic<std::size_t> ray_triangle_tests{0};
  std::atomic<std::size_t> ambiguous_count{0};
  std::atomic<std::size_t> fallback_count{0};

  ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, options.threads);
  const ParallelSamplingStats parallel_stats = parallelFor(
      locations.size(),
      parallel_options,
      [&](std::size_t index) {
        const BlockSampleLocation loc = locations[index];
        AdaptiveSDFBlock& block = block_set.blocks[loc.block_index];
        const Vector3 p = gridPointFromIndex(block, loc.local_index);
        const BVHSDFSampleResult sample =
            use_bvh
                ? BVHSDFSampler::sampleWithBVH(
                      mesh,
                      sampler.bvh(),
                      p,
                      sampler_options)
                : BVHSDFSampler::sampleBruteForce(
                      mesh,
                      p,
                      options.signed_distance);
        block.phi[loc.local_index] = sample.success ? sample.phi : 0.0;
        nearest_node_visits.fetch_add(
            sample.nearest.node_visits,
            std::memory_order_relaxed);
        nearest_triangle_tests.fetch_add(
            sample.nearest.triangle_tests,
            std::memory_order_relaxed);
        ray_node_visits.fetch_add(
            sample.ray.node_visits,
            std::memory_order_relaxed);
        ray_triangle_tests.fetch_add(
            sample.ray.triangle_tests,
            std::memory_order_relaxed);
        if (sample.ambiguous_sign) {
          ambiguous_count.fetch_add(1, std::memory_order_relaxed);
        }
        if (sample.fallback_sign) {
          fallback_count.fetch_add(1, std::memory_order_relaxed);
        }
      });

  stats.sample_count = locations.size();
  stats.threads_used = parallel_stats.threads_used;
  stats.sampling_time_ms = parallel_stats.elapsed_ms;
  stats.nearest_node_visits = nearest_node_visits.load();
  stats.nearest_triangle_tests = nearest_triangle_tests.load();
  stats.ray_node_visits = ray_node_visits.load();
  stats.ray_triangle_tests = ray_triangle_tests.load();
  stats.ambiguous_sign_count = ambiguous_count.load();
  stats.fallback_count = fallback_count.load();

  if (options.benchmark_brute_reference &&
      options.acceleration == SDFSamplingAcceleration::BVH) {
    const auto ref0 = std::chrono::steady_clock::now();
    volatile double checksum = 0.0;
    for (const BlockSampleLocation& loc : locations) {
      const AdaptiveSDFBlock& block = block_set.blocks[loc.block_index];
      const BVHSDFSampleResult sample =
          BVHSDFSampler::sampleBruteForce(
              mesh,
              gridPointFromIndex(block, loc.local_index),
              options.signed_distance);
      checksum += sample.phi;
    }
    (void)checksum;
    const auto ref1 = std::chrono::steady_clock::now();
    stats.brute_reference_time_ms =
        std::chrono::duration<double, std::milli>(ref1 - ref0).count();
    if (stats.sampling_time_ms > 0.0) {
      stats.speedup_vs_bruteforce =
          stats.brute_reference_time_ms / stats.sampling_time_ms;
    }
  }

  if (stats.ambiguous_sign_count > 0) {
    report.warnings.push_back(
        "MeshSign produced ambiguous adaptive block samples; ambiguous points "
        "were kept as unsigned distances or resolved through brute-force "
        "fallback.");
  }
  report.used_bvh = use_bvh;
  report.threads_used = stats.threads_used;
  report.acceleration_stats = stats;
}

BulkExactSamplingResult sampleMarkedBlocksExact(
    const TriangleMesh& mesh,
    const BVHSDFSampler& sampler,
    const BVHSDFSamplerOptions& sampler_options,
    const std::vector<std::size_t>& block_indices,
    int threads,
    AdaptiveSDFBlockSet& block_set) {
  std::vector<BlockSampleLocation> locations;
  for (const std::size_t block_index : block_indices) {
    AdaptiveSDFBlock& block = block_set.blocks[block_index];
    block.signed_distance = sampler_options.signed_distance;
    block.phi.resize(
        static_cast<std::size_t>(block.nx) *
        static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz));
    for (std::size_t local = 0; local < block.phi.size(); ++local) {
      locations.push_back({block_index, local});
    }
  }

  ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, threads);
  const bool use_bvh =
      sampler_options.acceleration == SDFSamplingAcceleration::BVH &&
      sampler.hasBVH();
  const ParallelSamplingStats parallel_stats = parallelFor(
      locations.size(),
      parallel_options,
      [&](std::size_t index) {
        const BlockSampleLocation loc = locations[index];
        AdaptiveSDFBlock& block = block_set.blocks[loc.block_index];
        const Vector3 p = gridPointFromIndex(block, loc.local_index);
        const BVHSDFSampleResult sample =
            use_bvh
                ? BVHSDFSampler::sampleWithBVH(
                      mesh,
                      sampler.bvh(),
                      p,
                      sampler_options)
                : BVHSDFSampler::sampleBruteForce(
                      mesh,
                      p,
                      sampler_options.signed_distance);
        block.phi[loc.local_index] = sample.success ? sample.phi : 0.0;
      });

  BulkExactSamplingResult result;
  result.sample_count = locations.size();
  result.time_ms = parallel_stats.elapsed_ms;
  result.threads_used = parallel_stats.threads_used;
  return result;
}

void sampleBlocksHierarchical(
    const TriangleMesh& mesh,
    AdaptiveSDFBlockSet& block_set,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport& report) {
  BuildAccelerationStats stats;
  stats.acceleration = options.acceleration;
  stats.threads_requested = std::max(1, options.threads);

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = options.acceleration;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.enable_counters =
      options.hierarchical_sampling.hierarchical_diagnostics;
  sampler_options.bvh_options.degenerate_area_epsilon =
      options.degenerate_area_epsilon;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options, &stats);
  const bool use_bvh =
      options.acceleration == SDFSamplingAcceleration::BVH && sampler.hasBVH();

  HierarchicalSamplingOptions hierarchical_options =
      options.hierarchical_sampling;
  hierarchical_options.enable_hierarchical_sampling = true;
  hierarchical_options.threads = std::max(1, options.threads);
  if (hierarchical_options.target_max_abs_error <= 0.0) {
    hierarchical_options.target_max_abs_error =
        options.target_near_surface_error;
  }

  HierarchicalBlockSamplingStats sampling_stats;
  sampling_stats.diagnostics.fine_sample_count = 0;
  const auto total0 = std::chrono::steady_clock::now();
  std::vector<std::uint8_t> bulk_exact_done(block_set.blocks.size(), 0);
  BulkExactSamplingResult bulk_exact;
  if (!hierarchical_options.hierarchical_diagnostics &&
      hierarchical_options.keep_near_surface_exact &&
      hierarchical_options.near_surface_mode == NearSurfaceSamplingMode::Exact) {
    std::vector<std::size_t> near_surface_indices;
    for (std::size_t block_index = 0; block_index < block_set.blocks.size();
         ++block_index) {
      if (block_set.blocks[block_index].near_surface) {
        near_surface_indices.push_back(block_index);
        bulk_exact_done[block_index] = 1;
      }
    }
    bulk_exact = sampleMarkedBlocksExact(
        mesh,
        sampler,
        sampler_options,
        near_surface_indices,
        hierarchical_options.threads,
        block_set);
  }
  std::vector<HierarchicalBlockSamplingResult> sampled_blocks(
      block_set.blocks.size());
  const auto sample_one = [&](std::size_t block_index) {
    if (bulk_exact_done[block_index] != 0) {
      return;
    }
    const AdaptiveSDFBlock& partitioned_block = block_set.blocks[block_index];
    sampled_blocks[block_index] = HierarchicalBlockSampler::sampleBlock(
        partitioned_block.bounds,
        partitioned_block.block_id,
        partitioned_block.octree_node_id,
        partitioned_block.level,
        options.block_resolution,
        options.signed_distance,
        partitioned_block.near_surface,
        sampler,
        hierarchical_options);
  };
  ParallelSamplingStats parallel_stats;
  if (hierarchical_options.hierarchical_diagnostics ||
      hierarchical_options.threads <= 1) {
    parallel_stats.threads_used = 1;
    const auto serial0 = std::chrono::steady_clock::now();
    for (std::size_t block_index = 0; block_index < block_set.blocks.size();
         ++block_index) {
      sample_one(block_index);
    }
    const auto serial1 = std::chrono::steady_clock::now();
    parallel_stats.elapsed_ms =
        std::chrono::duration<double, std::milli>(serial1 - serial0).count();
  } else {
    ParallelSamplingOptions parallel_options;
    parallel_options.threads = hierarchical_options.threads;
    parallel_stats =
        parallelFor(block_set.blocks.size(), parallel_options, sample_one);
  }

  for (std::size_t block_index = 0; block_index < block_set.blocks.size();
       ++block_index) {
    AdaptiveSDFBlock& partitioned_block = block_set.blocks[block_index];
    if (bulk_exact_done[block_index] != 0) {
      const std::size_t sample_count = partitioned_block.phi.size();
      ++sampling_stats.block_count;
      ++sampling_stats.exact_block_count;
      sampling_stats.exact_sample_count += sample_count;
      sampling_stats.diagnostics.total_block_count += 1;
      sampling_stats.diagnostics.near_surface_block_count += 1;
      sampling_stats.diagnostics.exact_block_count += 1;
      sampling_stats.diagnostics.fine_sample_count += sample_count;
      sampling_stats.diagnostics.exact_bvh_sample_count += sample_count;
      continue;
    }
    HierarchicalBlockSamplingResult sampled =
        std::move(sampled_blocks[block_index]);
    if (!sampled.success) {
      sampled.block = HierarchicalBlockSampler::sampleBlockExact(
          partitioned_block.bounds,
          partitioned_block.block_id,
          partitioned_block.octree_node_id,
          partitioned_block.level,
          options.block_resolution,
          options.signed_distance,
          partitioned_block.near_surface,
          sampler);
      sampled.fallback_exact = true;
      sampled.exact_sample_count += sampled.block.phi.size();
      sampling_stats.warnings.push_back(
          "hierarchical sampling failed for block " +
          std::to_string(partitioned_block.block_id) +
          " and exact fallback was used: " + sampled.error_message);
    }
    partitioned_block = std::move(sampled.block);
    ++sampling_stats.block_count;
    sampling_stats.exact_sample_count += sampled.exact_sample_count;
    sampling_stats.predicted_sample_count += sampled.predicted_sample_count;
    sampling_stats.quality_check_sample_count +=
        sampled.quality.check_sample_count;
    sampling_stats.exact_sampling_time_ms += sampled.exact_sampling_time_ms;
    sampling_stats.prediction_time_ms += sampled.prediction_time_ms;
    sampling_stats.quality_check_time_ms += sampled.quality_check_time_ms;
    sampling_stats.diagnostics.total_block_count +=
        sampled.diagnostics.total_block_count;
    sampling_stats.diagnostics.near_surface_block_count +=
        sampled.diagnostics.near_surface_block_count;
    sampling_stats.diagnostics.transition_block_count +=
        sampled.diagnostics.transition_block_count;
    sampling_stats.diagnostics.far_field_block_count +=
        sampled.diagnostics.far_field_block_count;
    sampling_stats.diagnostics.exact_block_count +=
        sampled.diagnostics.exact_block_count;
    sampling_stats.diagnostics.predicted_block_count +=
        sampled.diagnostics.predicted_block_count;
    sampling_stats.diagnostics.accepted_prediction_block_count +=
        sampled.diagnostics.accepted_prediction_block_count;
    sampling_stats.diagnostics.fallback_exact_block_count +=
        sampled.diagnostics.fallback_exact_block_count;
    sampling_stats.diagnostics.coarse_sample_count +=
        sampled.diagnostics.coarse_sample_count;
    sampling_stats.diagnostics.fine_sample_count +=
        sampled.diagnostics.fine_sample_count;
    sampling_stats.diagnostics.exact_bvh_sample_count +=
        sampled.diagnostics.exact_bvh_sample_count;
    sampling_stats.diagnostics.predicted_sample_count +=
        sampled.diagnostics.predicted_sample_count;
    sampling_stats.diagnostics.quality_check_sample_count +=
        sampled.diagnostics.quality_check_sample_count;
    sampling_stats.diagnostics.reused_coarse_sample_count +=
        sampled.diagnostics.reused_coarse_sample_count;
    sampling_stats.diagnostics.skipped_far_field_quality_check_count +=
        sampled.diagnostics.skipped_far_field_quality_check_count;
    sampling_stats.diagnostics.near_surface_banded_block_count +=
        sampled.diagnostics.near_surface_banded_block_count;
    sampling_stats.diagnostics.near_surface_local_exact_node_count +=
        sampled.diagnostics.near_surface_local_exact_node_count;
    sampling_stats.diagnostics.near_surface_predicted_node_count +=
        sampled.diagnostics.near_surface_predicted_node_count;
    sampling_stats.diagnostics.near_surface_local_fallback_node_count +=
        sampled.diagnostics.near_surface_local_fallback_node_count;
    sampling_stats.diagnostics.distance_query_count +=
        sampled.diagnostics.distance_query_count;
    sampling_stats.diagnostics.sign_query_count +=
        sampled.diagnostics.sign_query_count;
    sampling_stats.diagnostics.triangle_distance_test_count +=
        sampled.diagnostics.triangle_distance_test_count;
    sampling_stats.diagnostics.bvh_node_visit_count +=
        sampled.diagnostics.bvh_node_visit_count;
    sampling_stats.diagnostics.fallback_due_to_error_count +=
        sampled.diagnostics.fallback_due_to_error_count;
    sampling_stats.diagnostics.fallback_due_to_sign_count +=
        sampled.diagnostics.fallback_due_to_sign_count;
    sampling_stats.diagnostics.fallback_due_to_near_surface_count +=
        sampled.diagnostics.fallback_due_to_near_surface_count;
    sampling_stats.diagnostics.fallback_due_to_invalid_prediction_count +=
        sampled.diagnostics.fallback_due_to_invalid_prediction_count;
    sampling_stats.diagnostics.classification_time_ms +=
        sampled.diagnostics.classification_time_ms;
    sampling_stats.diagnostics.coarse_sampling_time_ms +=
        sampled.diagnostics.coarse_sampling_time_ms;
    sampling_stats.diagnostics.prediction_time_ms +=
        sampled.diagnostics.prediction_time_ms;
    sampling_stats.diagnostics.quality_check_time_ms +=
        sampled.diagnostics.quality_check_time_ms;
    sampling_stats.diagnostics.fallback_exact_time_ms +=
        sampled.diagnostics.fallback_exact_time_ms;
    sampling_stats.diagnostics.exact_sampling_time_ms +=
        sampled.diagnostics.exact_sampling_time_ms;
    if (sampled.decision.mode == BlockSamplingMode::ExactBVH) {
      ++sampling_stats.exact_block_count;
    } else {
      ++sampling_stats.predicted_block_count;
    }
    if (sampled.used_prediction) {
      ++sampling_stats.accepted_prediction_block_count;
    }
    if (sampled.fallback_exact) {
      ++sampling_stats.fallback_exact_block_count;
    }
    for (const std::string& warning : sampled.quality.warnings) {
      sampling_stats.warnings.push_back(
          "block " + std::to_string(partitioned_block.block_id) + ": " +
          warning);
    }
  }
  const auto total1 = std::chrono::steady_clock::now();
  sampling_stats.total_time_ms =
      std::chrono::duration<double, std::milli>(total1 - total0).count();
  const std::size_t exact_dense_samples = [&]() {
    std::size_t count = 0;
    for (const AdaptiveSDFBlock& block : block_set.blocks) {
      count += static_cast<std::size_t>(block.nx) *
               static_cast<std::size_t>(block.ny) *
               static_cast<std::size_t>(block.nz);
    }
    return count;
  }();
  if (sampling_stats.exact_sample_count > 0) {
    sampling_stats.speedup_vs_exact_estimate =
        static_cast<double>(exact_dense_samples) /
        static_cast<double>(sampling_stats.exact_sample_count);
  }
  if (sampling_stats.diagnostics.fine_sample_count == 0) {
    sampling_stats.diagnostics.fine_sample_count = exact_dense_samples;
  }
  sampling_stats.exact_sampling_time_ms += bulk_exact.time_ms;
  sampling_stats.diagnostics.exact_sampling_time_ms += bulk_exact.time_ms;
  sampling_stats.diagnostics.total_hierarchical_time_ms =
      sampling_stats.total_time_ms;
  finalizeHierarchicalSamplingDiagnostics(&sampling_stats.diagnostics);

  stats.sample_count = sampling_stats.exact_sample_count;
  stats.threads_used =
      std::max(parallel_stats.threads_used, bulk_exact.threads_used);
  stats.sampling_time_ms = sampling_stats.total_time_ms;

  report.used_bvh = use_bvh;
  report.threads_used = stats.threads_used;
  report.acceleration_stats = stats;
  report.hierarchical_sampling = sampling_stats;
  for (const std::string& warning : sampling_stats.warnings) {
    report.warnings.push_back(warning);
  }
}

void sampleBlocksContactBand(
    const TriangleMesh& mesh,
    AdaptiveSDFBlockSet& block_set,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport& report) {
  BuildAccelerationStats stats;
  stats.acceleration = options.acceleration;
  stats.threads_requested = std::max(1, options.threads);

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = options.acceleration;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.enable_counters = false;
  sampler_options.bvh_options.degenerate_area_epsilon =
      options.degenerate_area_epsilon;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options, &stats);
  const bool use_bvh =
      options.acceleration == SDFSamplingAcceleration::BVH && sampler.hasBVH();

  ContactBandDiagnostics diagnostics;
  const auto total0 = std::chrono::steady_clock::now();
  std::vector<ContactBandBlockSamplingResult> sampled_blocks(
      block_set.blocks.size());
  const auto sample_one = [&](std::size_t block_index) {
    const AdaptiveSDFBlock& partitioned_block = block_set.blocks[block_index];
    sampled_blocks[block_index] = ContactBandBlockSampler::sampleBlock(
        partitioned_block.bounds,
        partitioned_block.block_id,
        partitioned_block.octree_node_id,
        partitioned_block.level,
        options.block_resolution,
        options.signed_distance,
        sampler,
        sampler.bvh(),
        options.contact_band_sampling);
  };
  ParallelSamplingStats parallel_stats;
  if (options.threads <= 1) {
    parallel_stats.threads_used = 1;
    const auto serial0 = std::chrono::steady_clock::now();
    for (std::size_t block_index = 0; block_index < block_set.blocks.size();
         ++block_index) {
      sample_one(block_index);
    }
    const auto serial1 = std::chrono::steady_clock::now();
    parallel_stats.elapsed_ms =
        std::chrono::duration<double, std::milli>(serial1 - serial0).count();
  } else {
    ParallelSamplingOptions parallel_options;
    parallel_options.threads = std::max(1, options.threads);
    parallel_stats =
        parallelFor(block_set.blocks.size(), parallel_options, sample_one);
  }

  for (std::size_t block_index = 0; block_index < block_set.blocks.size();
       ++block_index) {
    ContactBandBlockSamplingResult sampled =
        std::move(sampled_blocks[block_index]);
    if (!sampled.success) {
      report.warnings.push_back(
          "contact-band sampling failed for block " +
          std::to_string(block_set.blocks[block_index].block_id) +
          ": " + sampled.error_message);
      continue;
    }
    block_set.blocks[block_index] = std::move(sampled.block);
    diagnostics.total_block_count += sampled.diagnostics.total_block_count;
    diagnostics.contact_band_block_count +=
        sampled.diagnostics.contact_band_block_count;
    diagnostics.far_field_block_count +=
        sampled.diagnostics.far_field_block_count;
    diagnostics.total_node_count += sampled.diagnostics.total_node_count;
    diagnostics.exact_node_count += sampled.diagnostics.exact_node_count;
    diagnostics.predicted_node_count +=
        sampled.diagnostics.predicted_node_count;
    diagnostics.far_field_node_count +=
        sampled.diagnostics.far_field_node_count;
    diagnostics.coarse_sample_count += sampled.diagnostics.coarse_sample_count;
    diagnostics.distance_query_count +=
        sampled.diagnostics.distance_query_count;
    diagnostics.sign_query_count += sampled.diagnostics.sign_query_count;
    diagnostics.exact_sampling_time_ms +=
        sampled.diagnostics.exact_sampling_time_ms;
    diagnostics.coarse_sampling_time_ms +=
        sampled.diagnostics.coarse_sampling_time_ms;
    diagnostics.interpolation_time_ms +=
        sampled.diagnostics.interpolation_time_ms;
    diagnostics.total_time_ms += sampled.diagnostics.total_time_ms;
  }
  const auto total1 = std::chrono::steady_clock::now();
  diagnostics.total_time_ms =
      std::chrono::duration<double, std::milli>(total1 - total0).count();
  finalizeContactBandDiagnostics(&diagnostics);

  stats.sample_count =
      diagnostics.exact_node_count + diagnostics.coarse_sample_count;
  stats.threads_used = parallel_stats.threads_used;
  stats.sampling_time_ms = diagnostics.total_time_ms;
  report.used_bvh = use_bvh;
  report.threads_used = stats.threads_used;
  report.acceleration_stats = stats;
  report.contact_band_sampling = diagnostics;
}

void assignReport(
    AdaptiveBlockSDFBuildReport* out,
    const AdaptiveBlockSDFBuildReport& value) {
  if (out != nullptr) {
    *out = value;
  }
}

TriangleMesh maybeCleanup(
    const TriangleMesh& input,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport& report) {
  if (!options.auto_safe_cleanup) {
    return input;
  }
  MeshCleanupOptions cleanup_options;
  cleanup_options.vertex_merge_tolerance = options.vertex_merge_tolerance;
  cleanup_options.degenerate_area_epsilon = options.degenerate_area_epsilon;
  const MeshCleanupResult cleanup = MeshCleanup::clean(input, cleanup_options);
  if (!cleanup.success) {
    report.warnings.push_back(
        "auto safe cleanup failed and original mesh was kept: " +
        cleanup.error_message);
    return input;
  }
  report.used_cleanup = true;
  for (const std::string& warning : cleanup.stats.warnings) {
    report.warnings.push_back("cleanup: " + warning);
  }
  return cleanup.cleaned_mesh;
}

}  // namespace

std::shared_ptr<SDFModel> AdaptiveBlockSDFBuilder::fromMesh(
    const TriangleMesh& mesh,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport* report_out) {
  AdaptiveBlockSDFBuildReport report;
  report.signed_distance = options.signed_distance;
  report.min_octree_level = options.min_octree_level;
  report.max_octree_level = options.max_octree_level;
  report.block_resolution = options.block_resolution;
  const auto t0 = std::chrono::steady_clock::now();

  if (mesh.empty()) {
    report.error_message = "AdaptiveBlockSDFBuilder requires a non-empty mesh.";
    assignReport(report_out, report);
    return nullptr;
  }
  if (options.block_resolution < 2) {
    report.error_message = "Adaptive block resolution must be at least 2.";
    assignReport(report_out, report);
    return nullptr;
  }
  if (options.min_octree_level < 0 ||
      options.max_octree_level < options.min_octree_level) {
    report.error_message = "Adaptive octree level range is invalid.";
    assignReport(report_out, report);
    return nullptr;
  }

  TriangleMesh working_mesh = maybeCleanup(mesh, options, report);

  MeshDiagnosticsOptions diagnostics_options;
  diagnostics_options.duplicate_triangle_tolerance =
      options.vertex_merge_tolerance;
  diagnostics_options.degenerate_area_epsilon = options.degenerate_area_epsilon;
  if (options.run_mesh_diagnostics) {
    report.diagnostics =
        MeshDiagnostics::analyze(working_mesh, diagnostics_options);
  } else {
    report.diagnostics.valid_mesh = !working_mesh.empty();
    report.diagnostics.vertex_count = working_mesh.vertexCount();
    report.diagnostics.triangle_count = working_mesh.triangleCount();
    report.diagnostics.raw_triangle_count = working_mesh.triangleCount();
    report.diagnostics.aabb = working_mesh.aabb();
    report.diagnostics.diagonal_length = working_mesh.diagonalLength();
  }

  MeshReadinessOptions readiness_options;
  readiness_options.require_watertight =
      options.signed_distance && options.require_watertight_for_signed;
  if (options.run_readiness_check) {
    report.readiness =
        MeshReadiness::evaluate(report.diagnostics, readiness_options);
  }
  report.watertight = report.diagnostics.watertight;

  if (options.signed_distance && options.require_watertight_for_signed &&
      !report.diagnostics.watertight) {
    report.error_message =
        "Signed AdaptiveBlockSDF build requires a watertight mesh. Use "
        "--unsigned for open meshes or repair/fill holes before signed build.";
    assignReport(report_out, report);
    return nullptr;
  }

  AdaptiveOctreeBuildOptions octree_options;
  octree_options.min_level = options.min_octree_level;
  octree_options.max_level = options.max_octree_level;
  octree_options.padding = options.padding;
  octree_options.target_near_surface_error =
      options.target_near_surface_error;
  octree_options.surface_band_factor = options.surface_band_factor;
  octree_options.signed_distance = options.signed_distance;
  octree_options.require_watertight_for_signed =
      options.require_watertight_for_signed;

  AdaptiveOctree octree =
      AdaptiveOctreeBuilder::build(working_mesh, octree_options, &report.octree_report);
  if (!report.octree_report.success) {
    report.error_message = report.octree_report.error_message;
    assignReport(report_out, report);
    return nullptr;
  }

  AdaptiveBlockPartitionOptions partition_options;
  partition_options.block_resolution = options.block_resolution;
  partition_options.include_all_leaves = true;
  AdaptiveSDFBlockSet blocks =
      AdaptiveBlockPartitioner::partition(octree, partition_options);
  blocks.signed_distance = options.signed_distance;

  const auto sample0 = std::chrono::steady_clock::now();
  if (options.contact_band_sampling.enable_contact_band_sampling) {
    sampleBlocksContactBand(working_mesh, blocks, options, report);
  } else if (options.hierarchical_sampling.enable_hierarchical_sampling) {
    sampleBlocksHierarchical(working_mesh, blocks, options, report);
  } else {
    sampleBlocks(working_mesh, blocks, options, report);
  }
  const auto sample1 = std::chrono::steady_clock::now();
  report.sampling_time_ms =
      std::chrono::duration<double, std::milli>(sample1 - sample0).count();
  report.acceleration_stats.sampling_time_ms = report.sampling_time_ms;

  auto model = std::make_shared<AdaptiveBlockSDFModel>(std::move(blocks));
  if (!model->isValid()) {
    report.error_message =
        "AdaptiveBlockSDFModel validation failed after build.";
    assignReport(report_out, report);
    return nullptr;
  }

  const auto t1 = std::chrono::steady_clock::now();
  report.success = true;
  report.max_octree_level_used = report.octree_report.max_level_used;
  report.octree_node_count = report.octree_report.node_count;
  report.octree_leaf_count = report.octree_report.leaf_count;
  report.block_count = model->blockSet().blockCount();
  report.near_surface_block_count =
      static_cast<std::size_t>(std::count_if(
          model->blockSet().blocks.begin(),
          model->blockSet().blocks.end(),
          [](const AdaptiveSDFBlock& block) { return block.near_surface; }));
  report.memory_bytes = model->memoryFootprintBytes();
  report.build_time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  assignReport(report_out, report);
  return model;
}

std::shared_ptr<SDFModel> AdaptiveBlockSDFBuilder::fromSTL(
    const std::string& stl_path,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport* report) {
  STLReadOptions read_options;
  read_options.vertex_merge_tolerance = options.vertex_merge_tolerance;
  const STLReadResult read = STLReader::read(stl_path, read_options);
  if (!read.success) {
    AdaptiveBlockSDFBuildReport failure;
    failure.signed_distance = options.signed_distance;
    failure.error_message = "Failed to read STL: " + read.error_message;
    assignReport(report, failure);
    return nullptr;
  }
  auto model = fromMesh(read.mesh, options, report);
  if (report != nullptr) {
    report->diagnostics.raw_triangle_count = read.raw_triangle_count;
  }
  if (model) {
    SDFMetadata metadata = model->metadata();
    metadata.source_path = stl_path;
    model->setMetadata(metadata);
  }
  return model;
}

}  // namespace adasdf
