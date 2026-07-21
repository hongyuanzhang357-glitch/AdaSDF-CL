#include "adasdf/narrowband/NarrowBandBrickSDFBuilder.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/narrowband/BrickTensorGenerator.h"
#include "adasdf/narrowband/CompressionBrickTree.h"
#include "adasdf/narrowband/ContactWeightedCompressor.h"
#include "adasdf/narrowband/SamplingOctree.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string dimKey(const CompressionBrick& brick) {
  return std::to_string(brick.tensor_nx) + "x" +
         std::to_string(brick.tensor_ny) + "x" +
         std::to_string(brick.tensor_nz);
}

void copySamplingStats(
    const SamplingOctreePlan& plan,
    NarrowBandBrickBuildStats* stats) {
  stats->sampling_node_count = plan.nodes.size();
  stats->sampling_node_count_by_level = plan.node_count_by_level;
  stats->exact_sample_estimate_by_level =
      plan.exact_sample_estimate_by_level;
  stats->interpolated_sample_estimate_by_level =
      plan.interpolated_sample_estimate_by_level;
  stats->contact_band_node_count_by_level =
      plan.contact_band_node_count_by_level;
  stats->far_field_node_count_by_level = plan.far_field_node_count_by_level;
}

}  // namespace

std::shared_ptr<AdaptiveBlockSDFModel> NarrowBandBrickSDFBuilder::fromMesh(
    const TriangleMesh& mesh,
    const NarrowBandBrickBuildOptions& options,
    NarrowBandBrickBuildStats* stats_out) {
  NarrowBandBrickBuildStats stats;
  const Clock::time_point total_start = Clock::now();
  if (mesh.empty()) {
    throw std::runtime_error("NarrowBandBrickSDFBuilder requires a non-empty mesh.");
  }
  if (options.brick_level_map.empty()) {
    throw std::runtime_error("NarrowBandBrickSDFBuilder requires brick-level-map.");
  }

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.fallback_to_bruteforce_sign = true;
  BuildAccelerationStats accel_stats;
  BVHSDFSampler sampler;
  const Clock::time_point bvh_start = Clock::now();
  if (!sampler.reset(mesh, sampler_options, &accel_stats)) {
    throw std::runtime_error("NarrowBandBrickSDFBuilder failed to build BVH sampler.");
  }
  stats.bvh_build_time_ms = elapsedMs(bvh_start, Clock::now());

  const Clock::time_point sampling_start = Clock::now();
  const SamplingOctreePlan sampling_plan =
      SamplingOctree::build(mesh, sampler, options);
  stats.sampling_tree_time_ms = elapsedMs(sampling_start, Clock::now());
  copySamplingStats(sampling_plan, &stats);

  const Clock::time_point brick_start = Clock::now();
  CompressionBrickTreePlan brick_plan =
      CompressionBrickTree::build(sampling_plan, options);
  stats.brick_planning_time_ms = elapsedMs(brick_start, Clock::now());
  stats.brick_count = brick_plan.bricks.size();
  stats.brick_count_by_level = brick_plan.brick_count_by_level;
  for (const CompressionBrick& brick : brick_plan.bricks) {
    if (brick.should_split) {
      ++stats.brick_split_recommendation_count;
    }
    if (brick.should_merge) {
      ++stats.brick_merge_recommendation_count;
    }
  }

  AdaptiveSDFBlockSet block_set;
  block_set.global_bounds = sampling_plan.bounds;
  block_set.signed_distance = true;
  const Clock::time_point tensor_start = Clock::now();
  for (const CompressionBrick& brick : brick_plan.bricks) {
    BrickTensorResult tensor =
        BrickTensorGenerator::generate(brick, sampler, options);
    stats.total_tensor_nodes += tensor.block.phi.size();
    stats.total_exact_source_nodes += tensor.brick.exact_source_node_count;
    stats.total_interpolated_fill_nodes +=
        tensor.brick.interpolated_fill_node_count;
    stats.sign_check_node_count += tensor.brick.sign_check_node_count;
    stats.sign_check_mismatch_count +=
        tensor.brick.sign_check_mismatch_count;
    stats.fill_fallback_exact_node_count +=
        tensor.brick.fill_fallback_exact_node_count;
    stats.zero_crossing_cell_count += tensor.brick.zero_crossing_cell_count;
    stats.protected_zero_crossing_cell_count +=
        tensor.brick.protected_zero_crossing_cell_count;
    stats.estimated_expanded_bytes += tensor.brick.estimated_expanded_bytes;
    stats.estimated_compressed_bytes += tensor.brick.estimated_compressed_bytes;
    stats.max_single_brick_expanded_bytes = std::max(
        stats.max_single_brick_expanded_bytes,
        tensor.brick.estimated_expanded_bytes);
    ++stats.tensor_dim_distribution[dimKey(tensor.brick)];
    block_set.blocks.push_back(std::move(tensor.block));
  }
  stats.tensor_generation_time_ms = elapsedMs(tensor_start, Clock::now());

  const Clock::time_point compression_start = Clock::now();
  ContactWeightedCompressionReport compression_report;
  block_set = ContactWeightedCompressor::compressCompatible(
      std::move(block_set),
      options,
      &compression_report);
  stats.compression_time_ms = elapsedMs(compression_start, Clock::now());
  stats.compressed_block_count = compression_report.compressed_block_count;
  stats.dense_fallback_block_count =
      compression_report.dense_fallback_block_count;
  stats.rank_min = compression_report.rank_min;
  stats.rank_mean = compression_report.rank_mean;
  stats.rank_max = compression_report.rank_max;
  stats.contact_band_sign_flip_count =
      compression_report.contact_band_sign_flip_count;
  stats.contact_band_p95_compression_error =
      compression_report.contact_band_p95_compression_error;
  stats.estimated_compressed_bytes =
      compression_report.estimated_compressed_bytes;
  stats.estimated_expanded_bytes =
      compression_report.estimated_expanded_bytes;
  stats.estimated_active_expanded_bytes = stats.estimated_expanded_bytes;
  stats.fill_sign_check_mismatch_count = stats.sign_check_mismatch_count;
  stats.zero_crossing_risk_cell_count = stats.zero_crossing_cell_count;

  auto model =
      std::make_shared<AdaptiveBlockSDFModel>(std::move(block_set));
  if (!model->isValid() || !model->queryBackendAvailable()) {
    throw std::runtime_error(
        "NarrowBandBrickSDFBuilder produced an invalid adaptive block model.");
  }
  stats.success = true;
  stats.sign_protected_fill_enabled = options.sign_protected_fill;
  stats.total_time_ms = elapsedMs(total_start, Clock::now());
  stats.warnings.push_back(
      "Prototype emits compatible adaptive block dense SDFBin; "
      "contact-weighted low-rank compression is reported as planning metadata.");
  stats.warnings.push_back(
      "contact-exact-far-interp currently uses exact BVH phi for emitted "
      "values and classifies far nodes for accounting.");
  if (stats_out != nullptr) {
    *stats_out = stats;
  }
  return model;
}

}  // namespace adasdf
