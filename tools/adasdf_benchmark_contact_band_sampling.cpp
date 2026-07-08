#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ModelJsonHelpers.h"

namespace {

struct Options {
  std::filesystem::path input;
  int max_level = 4;
  int min_level = 0;
  int block_resolution = 8;
  double target_error = 1e-3;
  int threads = 1;
  bool signed_distance = true;
  std::filesystem::path csv;
  std::filesystem::path report;
  std::filesystem::path json;
  std::filesystem::path marker_debug_csv;
  std::string case_id = "contact_band";
  bool marker_cost_audit = false;
  std::string timing_mode = "end-to-end";
  bool include_audit_in_wall_time = true;
  bool exclude_audit_from_speedup = false;
  bool include_marker_in_speedup = true;
  bool exclude_marker_from_speedup = false;
  adasdf::ContactBandSamplingOptions contact;
};

struct Location {
  std::size_t block_index = 0;
  std::size_t local_index = 0;
};

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

bool validTimingMode(const std::string& mode) {
  return mode == "end-to-end" || mode == "core" || mode == "diagnostic";
}

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_contact_band_sampling input.stl "
         "[--max-level N] [--block-resolution N] [--target-error value] "
         "[--contact-band-width value] [--contact-band-layers N] "
         "[--halo-exact-layers N] [--far-field-resolution N] "
         "[--far-field-mode coarse-interpolate|constant-sign|clamped-distance] "
         "[--contact-band-marker conservative-aabb|distance-aware|hybrid] "
         "[--marker-safety-factor value] [--marker-cell-size-factor value] "
         "[--marker-min-band value] [--marker-max-band value] "
         "[--disable-global-halo] [--local-halo-only] "
         "[--reuse-far-field-sign] [--no-reuse-far-field-sign] "
         "[--coverage-audit] [--coverage-samples-per-axis N] "
         "[--marker-cost-audit] [--save-marker-debug-csv out.csv] "
         "[--timing-mode end-to-end|core|diagnostic] "
         "[--include-audit-in-wall-time] [--exclude-audit-from-speedup] "
         "[--include-marker-in-speedup] [--exclude-marker-from-speedup] "
         "[--normal-audit] [--normal-error-limit-deg value] "
         "[--threads N] [--csv out.csv] [--report out.md] "
         "[--json [out.json]] [--case-id id]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::Vector3 gridPoint(
    const adasdf::AdaptiveSDFBlock& block,
    std::size_t index) {
  const int i = static_cast<int>(index % static_cast<std::size_t>(block.nx));
  const int j = static_cast<int>(
      (index / static_cast<std::size_t>(block.nx)) %
      static_cast<std::size_t>(block.ny));
  const int k = static_cast<int>(
      index /
      (static_cast<std::size_t>(block.nx) *
       static_cast<std::size_t>(block.ny)));
  return {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
}

std::vector<adasdf::AdaptiveSDFBlock> makeBlocks(
    const adasdf::TriangleMesh& mesh,
    const Options& options) {
  adasdf::AdaptiveOctreeBuildOptions octree_options;
  octree_options.min_level = options.min_level;
  octree_options.max_level = options.max_level;
  octree_options.target_near_surface_error = options.target_error;
  octree_options.signed_distance = options.signed_distance;
  octree_options.require_watertight_for_signed = false;
  adasdf::AdaptiveOctree octree =
      adasdf::AdaptiveOctreeBuilder::build(mesh, octree_options);
  adasdf::AdaptiveBlockPartitionOptions partition_options;
  partition_options.block_resolution = options.block_resolution;
  partition_options.include_all_leaves = true;
  return adasdf::AdaptiveBlockPartitioner::partition(octree, partition_options)
      .blocks;
}

double sampleReferenceExact(
    const adasdf::TriangleMesh& mesh,
    const adasdf::BVHSDFSampler& sampler,
    const adasdf::BVHSDFSamplerOptions& sampler_options,
    int threads,
    std::vector<adasdf::AdaptiveSDFBlock>* blocks) {
  std::vector<Location> locations;
  for (std::size_t block_index = 0; block_index < blocks->size(); ++block_index) {
    adasdf::AdaptiveSDFBlock& block = (*blocks)[block_index];
    block.signed_distance = sampler_options.signed_distance;
    block.phi.assign(
        static_cast<std::size_t>(block.nx) * static_cast<std::size_t>(block.ny) *
            static_cast<std::size_t>(block.nz),
        0.0);
    for (std::size_t local = 0; local < block.phi.size(); ++local) {
      locations.push_back({block_index, local});
    }
  }
  adasdf::ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, threads);
  const bool use_bvh =
      sampler_options.acceleration == adasdf::SDFSamplingAcceleration::BVH &&
      sampler.hasBVH();
  const adasdf::ParallelSamplingStats stats = adasdf::parallelFor(
      locations.size(),
      parallel_options,
      [&](std::size_t index) {
        const Location loc = locations[index];
        adasdf::AdaptiveSDFBlock& block = (*blocks)[loc.block_index];
        const adasdf::Vector3 point = gridPoint(block, loc.local_index);
        const adasdf::BVHSDFSampleResult sample =
            use_bvh
                ? adasdf::BVHSDFSampler::sampleWithBVH(
                      mesh,
                      sampler.bvh(),
                      point,
                      sampler_options)
                : adasdf::BVHSDFSampler::sampleBruteForce(
                      mesh,
                      point,
                      sampler_options.signed_distance);
        block.phi[loc.local_index] = sample.success ? sample.phi : 0.0;
      });
  return stats.elapsed_ms;
}

void writeMarkerDebugCsv(
    const std::filesystem::path& path,
    const std::vector<adasdf::ContactBandBlockSamplingResult>& sampled) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path);
  out
      << "block_id,has_contact_band,candidate_cell_count,"
         "candidate_triangle_count,refined_candidate_count,"
         "rejected_candidate_count,accepted_contact_cell_count,"
         "marked_node_count,exact_node_count,marker_time_ms,"
         "triangle_bvh_query_time_ms,box_triangle_distance_time_ms,"
         "marker_false_positive_proxy\n";
  for (const auto& block : sampled) {
    const auto& d = block.diagnostics;
    out << block.block.block_id << ","
        << (block.has_contact_band ? "true" : "false") << ","
        << d.candidate_cell_count << "," << d.candidate_triangle_count
        << "," << d.refined_candidate_count << ","
        << d.rejected_candidate_count << ","
        << d.accepted_contact_cell_count << ","
        << d.marked_node_count << "," << d.exact_node_count << ","
        << d.marker_time_ms << "," << d.triangle_bvh_query_time_ms << ","
        << d.box_triangle_distance_time_ms << ","
        << d.marker_false_positive_proxy << "\n";
  }
}

adasdf::ContactBandBenchmarkResult runBenchmark(
    const adasdf::TriangleMesh& mesh,
    const Options& options) {
  adasdf::ContactBandBenchmarkResult result;
  result.case_id = options.case_id;
  result.timing_mode = options.timing_mode;
  result.include_audit_in_wall_time = options.include_audit_in_wall_time;
  result.include_marker_in_speedup = options.include_marker_in_speedup;
  result.exclude_audit_from_speedup = options.exclude_audit_from_speedup;
  result.exclude_marker_from_speedup = options.exclude_marker_from_speedup;

  std::vector<adasdf::AdaptiveSDFBlock> reference_blocks =
      makeBlocks(mesh, options);
  std::vector<adasdf::AdaptiveSDFBlock> contact_blocks = reference_blocks;

  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.enable_counters = false;
  adasdf::BuildAccelerationStats acceleration_stats;
  acceleration_stats.threads_requested = std::max(1, options.threads);
  adasdf::BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options, &acceleration_stats);

  result.exact_reference_time_ms =
      sampleReferenceExact(
          mesh,
          sampler,
          sampler_options,
          options.threads,
          &reference_blocks);
  result.exact_reference_wall_time_ms = result.exact_reference_time_ms;
  result.exact_reference_core_build_time_ms =
      result.exact_reference_time_ms;

  std::vector<adasdf::ContactBandBlockSamplingResult> sampled(
      contact_blocks.size());
  const auto core0 = std::chrono::steady_clock::now();
  adasdf::ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, options.threads);
  adasdf::parallelFor(
      contact_blocks.size(),
      parallel_options,
      [&](std::size_t block_index) {
        const adasdf::AdaptiveSDFBlock& block = contact_blocks[block_index];
        sampled[block_index] = adasdf::ContactBandBlockSampler::sampleBlock(
            block.bounds,
            block.block_id,
            block.octree_node_id,
            block.level,
            options.block_resolution,
            options.signed_distance,
            sampler,
            sampler.bvh(),
            options.contact);
      });
  const auto core1 = std::chrono::steady_clock::now();
  result.contact_band_core_build_time_ms = elapsedMs(core0, core1);

  adasdf::ContactBandDiagnostics diagnostics;
  diagnostics.marker_mode = adasdf::toString(options.contact.marker_mode);
  const auto diagnostics0 = std::chrono::steady_clock::now();
  for (std::size_t block_index = 0; block_index < sampled.size(); ++block_index) {
    const adasdf::ContactBandBlockSamplingResult& block_result =
        sampled[block_index];
    if (!block_result.success) {
      result.error_message = block_result.error_message;
      return result;
    }
    contact_blocks[block_index] = block_result.block;
    diagnostics.total_block_count += block_result.diagnostics.total_block_count;
    diagnostics.contact_band_block_count +=
        block_result.diagnostics.contact_band_block_count;
    diagnostics.far_field_block_count +=
        block_result.diagnostics.far_field_block_count;
    diagnostics.total_node_count += block_result.diagnostics.total_node_count;
    diagnostics.exact_node_count += block_result.diagnostics.exact_node_count;
    diagnostics.predicted_node_count +=
        block_result.diagnostics.predicted_node_count;
    diagnostics.far_field_node_count +=
        block_result.diagnostics.far_field_node_count;
    diagnostics.coarse_sample_count +=
        block_result.diagnostics.coarse_sample_count;
    diagnostics.distance_query_count +=
        block_result.diagnostics.distance_query_count;
    diagnostics.sign_query_count += block_result.diagnostics.sign_query_count;
    diagnostics.candidate_triangle_aabb_overlap_count +=
        block_result.diagnostics.candidate_triangle_aabb_overlap_count;
    diagnostics.candidate_cell_count +=
        block_result.diagnostics.candidate_cell_count;
    diagnostics.candidate_triangle_count +=
        block_result.diagnostics.candidate_triangle_count;
    diagnostics.refined_candidate_count +=
        block_result.diagnostics.refined_candidate_count;
    diagnostics.rejected_candidate_count +=
        block_result.diagnostics.rejected_candidate_count;
    diagnostics.accepted_contact_cell_count +=
        block_result.diagnostics.accepted_contact_cell_count;
    diagnostics.distance_refined_cell_count +=
        block_result.diagnostics.distance_refined_cell_count;
    diagnostics.distance_rejected_cell_count +=
        block_result.diagnostics.distance_rejected_cell_count;
    diagnostics.marked_cell_count +=
        block_result.diagnostics.marked_cell_count;
    diagnostics.marked_node_count +=
        block_result.diagnostics.marked_node_count;
    diagnostics.local_halo_node_count +=
        block_result.diagnostics.local_halo_node_count;
    diagnostics.global_halo_node_count +=
        block_result.diagnostics.global_halo_node_count;
    diagnostics.exact_sampling_time_ms +=
        block_result.diagnostics.exact_sampling_time_ms;
    diagnostics.coarse_sampling_time_ms +=
        block_result.diagnostics.coarse_sampling_time_ms;
    diagnostics.interpolation_time_ms +=
        block_result.diagnostics.interpolation_time_ms;
    diagnostics.total_time_ms += block_result.diagnostics.total_time_ms;
    diagnostics.marker_time_ms += block_result.diagnostics.marker_time_ms;
    diagnostics.distance_refinement_time_ms +=
        block_result.diagnostics.distance_refinement_time_ms;
    diagnostics.marker_refinement_time_ms +=
        block_result.diagnostics.marker_refinement_time_ms;
    diagnostics.box_triangle_distance_time_ms +=
        block_result.diagnostics.box_triangle_distance_time_ms;
    diagnostics.triangle_bvh_query_time_ms +=
        block_result.diagnostics.triangle_bvh_query_time_ms;
  }
  const auto diagnostics1 = std::chrono::steady_clock::now();
  diagnostics.contact_band_diagnostics_time_ms =
      elapsedMs(diagnostics0, diagnostics1);

  adasdf::ContactBandQualityMetrics quality;
  const auto audit0 = std::chrono::steady_clock::now();
  for (std::size_t block_index = 0; block_index < sampled.size(); ++block_index) {
    const adasdf::ContactBandBlockSamplingResult& block_result =
        sampled[block_index];
    quality = adasdf::ContactBandQualityAudit::merge(
        quality,
        adasdf::ContactBandQualityAudit::auditBlock(
            contact_blocks[block_index],
            reference_blocks[block_index],
            block_result.mask,
            options.contact));
  }
  adasdf::ContactBandQualityAudit::finalize(&quality, options.contact);
  const auto audit1 = std::chrono::steady_clock::now();
  diagnostics.contact_band_audit_time_ms = elapsedMs(audit0, audit1);

  const double accumulated_block_time_ms = diagnostics.total_time_ms;
  if (accumulated_block_time_ms > 0.0) {
    const double marker_fraction =
        std::max(0.0, std::min(1.0, diagnostics.marker_time_ms /
                                        accumulated_block_time_ms));
    const double sampling_work_time_ms =
        diagnostics.exact_sampling_time_ms + diagnostics.coarse_sampling_time_ms;
    const double sampling_fraction =
        std::max(0.0, std::min(1.0, sampling_work_time_ms /
                                        accumulated_block_time_ms));
    const double interpolation_fraction =
        std::max(0.0, std::min(1.0, diagnostics.interpolation_time_ms /
                                        accumulated_block_time_ms));
    diagnostics.contact_band_marker_time_ms =
        result.contact_band_core_build_time_ms * marker_fraction;
    diagnostics.contact_band_sampling_time_ms =
        result.contact_band_core_build_time_ms * sampling_fraction;
    diagnostics.contact_band_interpolation_time_ms =
        result.contact_band_core_build_time_ms * interpolation_fraction;
  } else {
    diagnostics.contact_band_marker_time_ms = diagnostics.marker_time_ms;
    diagnostics.contact_band_sampling_time_ms =
        diagnostics.exact_sampling_time_ms + diagnostics.coarse_sampling_time_ms;
    diagnostics.contact_band_interpolation_time_ms =
        diagnostics.interpolation_time_ms;
  }
  result.contact_band_wall_time_ms =
      result.contact_band_core_build_time_ms +
      (options.include_audit_in_wall_time
           ? diagnostics.contact_band_audit_time_ms
           : 0.0);
  result.contact_band_time_ms = result.contact_band_wall_time_ms;
  diagnostics.total_time_ms = result.contact_band_wall_time_ms;
  adasdf::finalizeContactBandDiagnostics(&diagnostics);
  if (accumulated_block_time_ms > 0.0) {
    diagnostics.marker_time_fraction =
        diagnostics.marker_time_ms / accumulated_block_time_ms;
  }
  if (options.marker_cost_audit || !options.marker_debug_csv.empty()) {
    writeMarkerDebugCsv(options.marker_debug_csv, sampled);
  }
  result.diagnostics = diagnostics;
  result.quality = quality;
  result.speedup_end_to_end =
      result.contact_band_wall_time_ms > 0.0
          ? result.exact_reference_wall_time_ms / result.contact_band_wall_time_ms
          : 0.0;
  result.speedup = result.speedup_end_to_end;
  result.speedup_core_build =
      result.contact_band_core_build_time_ms > 0.0
          ? result.exact_reference_core_build_time_ms /
                result.contact_band_core_build_time_ms
          : 0.0;
  result.effective_speedup_including_marker = result.speedup_end_to_end;
  result.speedup_excluding_audit =
      result.contact_band_core_build_time_ms > 0.0
          ? result.exact_reference_wall_time_ms /
                result.contact_band_core_build_time_ms
          : 0.0;
  result.speedup_excluding_diagnostics = result.speedup_end_to_end;
  const double no_marker_time =
      result.contact_band_wall_time_ms -
      result.diagnostics.contact_band_marker_time_ms;
  result.speedup_excluding_marker =
      no_marker_time > 0.0
          ? result.exact_reference_wall_time_ms / no_marker_time
          : 0.0;
  result.effective_speedup_excluding_marker =
      result.speedup_excluding_marker;
  result.marker_time_fraction_of_wall =
      result.contact_band_wall_time_ms > 0.0
          ? result.diagnostics.contact_band_marker_time_ms /
                result.contact_band_wall_time_ms
          : 0.0;
  result.audit_time_fraction_of_wall =
      result.contact_band_wall_time_ms > 0.0
          ? result.diagnostics.contact_band_audit_time_ms /
                result.contact_band_wall_time_ms
          : 0.0;
  result.diagnostics_time_fraction_of_wall =
      result.contact_band_wall_time_ms > 0.0
          ? result.diagnostics.contact_band_diagnostics_time_ms /
                result.contact_band_wall_time_ms
          : 0.0;
  result.diagnostics.marker_time_fraction_of_wall =
      result.marker_time_fraction_of_wall;
  result.diagnostics.audit_time_fraction_of_wall =
      result.audit_time_fraction_of_wall;
  result.diagnostics.diagnostics_time_fraction_of_wall =
      result.diagnostics_time_fraction_of_wall;
  result.performance_claim_allowed =
      result.timing_mode == "end-to-end" &&
      result.include_audit_in_wall_time &&
      result.include_marker_in_speedup &&
      !result.exclude_audit_from_speedup &&
      !result.exclude_marker_from_speedup &&
      result.speedup_end_to_end > 1.0 &&
      result.quality.contact_band_quality_passed &&
      result.quality.coverage_passed &&
      result.quality.contact_band_sign_mismatch_count == 0 &&
      result.quality.near_surface_sign_mismatch_count == 0 &&
      result.quality.normal_flip_count == 0 &&
      result.quality.near_surface_normal_flip_count == 0;
  result.effective_speedup_claim_allowed =
      result.performance_claim_allowed;
  result.success = true;
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }
    Options options;
    options.contact.enable_contact_band_sampling = true;
    options.contact.normal_audit = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_level = std::stoi(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.min_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.target_error = std::stod(argv[++i]);
      } else if (arg == "--contact-band-width" && hasValue(i, argc)) {
        options.contact.contact_band_width = std::stod(argv[++i]);
      } else if (arg == "--contact-band-layers" && hasValue(i, argc)) {
        options.contact.contact_band_layers = std::stoi(argv[++i]);
      } else if (arg == "--halo-exact-layers" && hasValue(i, argc)) {
        options.contact.halo_exact_layers = std::stoi(argv[++i]);
      } else if (arg == "--far-field-resolution" && hasValue(i, argc)) {
        options.contact.far_field_resolution = std::stoi(argv[++i]);
      } else if (arg == "--far-field-mode" && hasValue(i, argc)) {
        adasdf::ContactBandFarFieldMode mode;
        if (!adasdf::parseContactBandFarFieldMode(argv[++i], &mode)) {
          std::cerr << "Unknown far-field mode: " << argv[i] << "\n";
          return 1;
        }
        options.contact.far_field_mode = mode;
      } else if (arg == "--contact-band-marker" && hasValue(i, argc)) {
        adasdf::ContactBandMarkerMode mode;
        if (!adasdf::parseContactBandMarkerMode(argv[++i], &mode)) {
          std::cerr << "Unknown contact-band marker: " << argv[i] << "\n";
          return 1;
        }
        options.contact.marker_mode = mode;
      } else if (arg == "--marker-safety-factor" && hasValue(i, argc)) {
        options.contact.marker_safety_factor = std::stod(argv[++i]);
      } else if (arg == "--marker-cell-size-factor" && hasValue(i, argc)) {
        options.contact.marker_cell_size_factor = std::stod(argv[++i]);
      } else if (arg == "--marker-min-band" && hasValue(i, argc)) {
        options.contact.marker_min_band = std::stod(argv[++i]);
      } else if (arg == "--marker-max-band" && hasValue(i, argc)) {
        options.contact.marker_max_band = std::stod(argv[++i]);
      } else if (arg == "--disable-global-halo") {
        options.contact.disable_global_halo = true;
      } else if (arg == "--local-halo-only") {
        options.contact.local_halo_only = true;
      } else if (arg == "--reuse-far-field-sign") {
        options.contact.reuse_far_field_sign = true;
      } else if (arg == "--no-reuse-far-field-sign") {
        options.contact.reuse_far_field_sign = false;
      } else if (arg == "--coverage-audit") {
        options.contact.coverage_audit = true;
      } else if (arg == "--coverage-samples-per-axis" && hasValue(i, argc)) {
        options.contact.coverage_samples_per_axis = std::stoi(argv[++i]);
      } else if (arg == "--marker-cost-audit") {
        options.marker_cost_audit = true;
      } else if (arg == "--save-marker-debug-csv" && hasValue(i, argc)) {
        options.marker_debug_csv = argv[++i];
      } else if (arg == "--timing-mode" && hasValue(i, argc)) {
        options.timing_mode = argv[++i];
        if (!validTimingMode(options.timing_mode)) {
          std::cerr << "Unknown timing mode: " << options.timing_mode << "\n";
          return 1;
        }
      } else if (arg == "--include-audit-in-wall-time") {
        options.include_audit_in_wall_time = true;
      } else if (arg == "--exclude-audit-from-speedup") {
        options.exclude_audit_from_speedup = true;
      } else if (arg == "--include-marker-in-speedup") {
        options.include_marker_in_speedup = true;
        options.exclude_marker_from_speedup = false;
      } else if (arg == "--exclude-marker-from-speedup") {
        options.include_marker_in_speedup = false;
        options.exclude_marker_from_speedup = true;
      } else if (arg == "--normal-audit" ||
                 arg == "--contact-band-normal-audit") {
        options.contact.normal_audit = true;
      } else if ((arg == "--normal-error-limit-deg" ||
                  arg == "--contact-band-normal-error-limit-deg") &&
                 hasValue(i, argc)) {
        options.contact.contact_band_normal_error_limit_deg =
            std::stod(argv[++i]);
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = std::stoi(argv[++i]);
      } else if (arg == "--unsigned") {
        options.signed_distance = false;
      } else if (arg == "--csv" && hasValue(i, argc)) {
        options.csv = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        options.report = argv[++i];
      } else if (arg == "--json") {
        if (hasValue(i, argc) && std::string(argv[i + 1]).rfind("-", 0) != 0) {
          options.json = argv[++i];
        } else {
          options.json = "-";
        }
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        options.case_id = argv[++i];
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (options.input.empty()) {
        options.input = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (options.input.empty() || !std::filesystem::exists(options.input)) {
      std::cerr
          << "adasdf_benchmark_contact_band_sampling: input STL missing: "
          << options.input.string() << "\n";
      return 1;
    }
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(options.input.string());
    if (!read.success) {
      std::cerr
          << "adasdf_benchmark_contact_band_sampling: failed to read STL: "
          << read.error_message << "\n";
      return 2;
    }

    adasdf::ContactBandBenchmarkResult result =
        runBenchmark(read.mesh, options);
    adasdf::ContactBandReportWriter::writeCsv(options.csv.string(), result);
    adasdf::ContactBandReportWriter::writeMarkdown(
        options.report.string(),
        result);
    if (!result.success) {
      std::cerr
          << "adasdf_benchmark_contact_band_sampling: benchmark failed: "
          << result.error_message << "\n";
      return 3;
    }

    auto make_json = [&]() {
      const double query_count =
          static_cast<double>(result.diagnostics.distance_query_count +
                              result.diagnostics.sign_query_count);
      adasdf::BackendJsonContract contract = adasdf_tools::makeBaseContract(
          adasdf::SchemaIds::Benchmark,
          "adasdf_benchmark_contact_band_sampling");
      contract.payload_fields.push_back(
          {"sample_count",
           adasdf::JsonContractWriter::integer(
               result.diagnostics.total_node_count)});
      contract.payload_fields.push_back(
          {"query_count", adasdf::JsonContractWriter::number(query_count)});
      contract.payload_fields.push_back(
          {"time_ms",
           adasdf::JsonContractWriter::number(
               result.contact_band_wall_time_ms)});
      contract.payload_fields.push_back(
          {"ns_per_query",
           adasdf::JsonContractWriter::number(
               query_count > 0.0
                   ? result.contact_band_wall_time_ms * 1.0e6 / query_count
                   : 0.0)});
      contract.payload_fields.push_back(
          {"throughput",
           adasdf::JsonContractWriter::number(
               result.contact_band_wall_time_ms > 0.0
                   ? query_count / (result.contact_band_wall_time_ms / 1000.0)
                   : 0.0)});
      contract.payload_fields.push_back(
          {"backend", adasdf::JsonContractWriter::quote("cpu")});
      contract.payload_fields.push_back(
          {"model_type",
           adasdf::JsonContractWriter::quote("mesh_contact_band")});
      contract.payload_fields.push_back(
          {"mode",
           adasdf::JsonContractWriter::quote("contact_band_sampling")});
      contract.payload_fields.push_back(
          {"contact_band_blocks",
           adasdf::JsonContractWriter::integer(
               result.diagnostics.contact_band_block_count)});
      contract.payload_fields.push_back(
          {"block_count",
           adasdf::JsonContractWriter::integer(
               result.diagnostics.total_block_count)});
      contract.payload_fields.push_back(
          {"speedup",
           adasdf::JsonContractWriter::number(result.speedup_end_to_end)});
      return adasdf::JsonContractWriter::writeObject(contract);
    };

    if (options.json == "-") {
      std::cout << make_json();
      return 0;
    }
    if (!options.json.empty()) {
      if (!options.json.parent_path().empty()) {
        std::filesystem::create_directories(options.json.parent_path());
      }
      std::ofstream file(options.json);
      if (!file) {
        std::cerr
            << "adasdf_benchmark_contact_band_sampling: failed to write JSON\n";
        return 3;
      }
      file << make_json();
    }

    std::cout << "AdaSDF-CL contact-band sampling benchmark\n";
    std::cout << "Case id: " << result.case_id << "\n";
    std::cout << "Timing mode: " << result.timing_mode << "\n";
    std::cout << "Include audit in wall time: "
              << (result.include_audit_in_wall_time ? "yes" : "no") << "\n";
    std::cout << "Include marker in speedup: "
              << (result.include_marker_in_speedup ? "yes" : "no") << "\n";
    std::cout << "Exclude audit from speedup: "
              << (result.exclude_audit_from_speedup ? "yes" : "no") << "\n";
    std::cout << "Exclude marker from speedup: "
              << (result.exclude_marker_from_speedup ? "yes" : "no") << "\n";
    std::cout << "Exact reference time ms: "
              << result.exact_reference_time_ms << "\n";
    std::cout << "Contact-band time ms: " << result.contact_band_time_ms
              << "\n";
    std::cout << "Speedup: " << result.speedup << "\n";
    std::cout << "Exact reference wall time ms: "
              << result.exact_reference_wall_time_ms << "\n";
    std::cout << "Contact-band wall time ms: "
              << result.contact_band_wall_time_ms << "\n";
    std::cout << "Speedup end-to-end: "
              << result.speedup_end_to_end << "\n";
    std::cout << "Exact reference core build time ms: "
              << result.exact_reference_core_build_time_ms << "\n";
    std::cout << "Contact-band core build time ms: "
              << result.contact_band_core_build_time_ms << "\n";
    std::cout << "Speedup core build: "
              << result.speedup_core_build << "\n";
    std::cout << "Effective speedup including marker: "
              << result.effective_speedup_including_marker << "\n";
    std::cout << "Effective speedup excluding marker: "
              << result.effective_speedup_excluding_marker << "\n";
    std::cout << "Speedup excluding audit: "
              << result.speedup_excluding_audit << "\n";
    std::cout << "Speedup excluding diagnostics: "
              << result.speedup_excluding_diagnostics << "\n";
    std::cout << "Speedup excluding marker: "
              << result.speedup_excluding_marker << "\n";
    std::cout << "Marker mode: " << result.diagnostics.marker_mode << "\n";
    std::cout << "Total blocks: "
              << result.diagnostics.total_block_count << "\n";
    std::cout << "Contact-band blocks: "
              << result.diagnostics.contact_band_block_count << "\n";
    std::cout << "Far-field blocks: "
              << result.diagnostics.far_field_block_count << "\n";
    std::cout << "Contact-band block ratio: "
              << result.diagnostics.contact_band_block_ratio << "\n";
    std::cout << "Total nodes: " << result.diagnostics.total_node_count
              << "\n";
    std::cout << "Exact nodes: " << result.diagnostics.exact_node_count
              << "\n";
    std::cout << "Predicted nodes: "
              << result.diagnostics.predicted_node_count << "\n";
    std::cout << "Far-field nodes: "
              << result.diagnostics.far_field_node_count << "\n";
    std::cout << "Coarse samples: "
              << result.diagnostics.coarse_sample_count << "\n";
    std::cout << "Exact node ratio: "
              << result.diagnostics.exact_node_ratio << "\n";
    std::cout << "Predicted node ratio: "
              << result.diagnostics.predicted_node_ratio << "\n";
    std::cout << "Exact sample reduction ratio: "
              << result.diagnostics.exact_sample_reduction_ratio << "\n";
    std::cout << "Distance query count: "
              << result.diagnostics.distance_query_count << "\n";
    std::cout << "Sign query count: "
              << result.diagnostics.sign_query_count << "\n";
    std::cout << "Sign query reduction ratio: "
              << result.diagnostics.sign_query_reduction_ratio << "\n";
    std::cout << "Candidate triangle AABB overlaps: "
              << result.diagnostics.candidate_triangle_aabb_overlap_count
              << "\n";
    std::cout << "Candidate cells: "
              << result.diagnostics.candidate_cell_count << "\n";
    std::cout << "Candidate triangles: "
              << result.diagnostics.candidate_triangle_count << "\n";
    std::cout << "Refined candidates: "
              << result.diagnostics.refined_candidate_count << "\n";
    std::cout << "Rejected candidates: "
              << result.diagnostics.rejected_candidate_count << "\n";
    std::cout << "Accepted contact cells: "
              << result.diagnostics.accepted_contact_cell_count << "\n";
    std::cout << "Marker false-positive proxy: "
              << result.diagnostics.marker_false_positive_proxy << "\n";
    std::cout << "Distance-refined cells: "
              << result.diagnostics.distance_refined_cell_count << "\n";
    std::cout << "Distance-rejected cells: "
              << result.diagnostics.distance_rejected_cell_count << "\n";
    std::cout << "Marked cells: "
              << result.diagnostics.marked_cell_count << "\n";
    std::cout << "Marked nodes: "
              << result.diagnostics.marked_node_count << "\n";
    std::cout << "Local halo nodes: "
              << result.diagnostics.local_halo_node_count << "\n";
    std::cout << "Global halo nodes: "
              << result.diagnostics.global_halo_node_count << "\n";
    std::cout << "Overmark ratio estimate: "
              << result.diagnostics.overmark_ratio_estimate << "\n";
    std::cout << "Marker time ms: "
              << result.diagnostics.marker_time_ms << "\n";
    std::cout << "Contact-band marker time ms: "
              << result.diagnostics.contact_band_marker_time_ms << "\n";
    std::cout << "Contact-band sampling time ms: "
              << result.diagnostics.contact_band_sampling_time_ms << "\n";
    std::cout << "Contact-band interpolation time ms: "
              << result.diagnostics.contact_band_interpolation_time_ms << "\n";
    std::cout << "Contact-band audit time ms: "
              << result.diagnostics.contact_band_audit_time_ms << "\n";
    std::cout << "Contact-band report time ms: "
              << result.diagnostics.contact_band_report_time_ms << "\n";
    std::cout << "Contact-band diagnostics time ms: "
              << result.diagnostics.contact_band_diagnostics_time_ms << "\n";
    std::cout << "Marker time fraction: "
              << result.diagnostics.marker_time_fraction << "\n";
    std::cout << "Marker time fraction of wall: "
              << result.marker_time_fraction_of_wall << "\n";
    std::cout << "Audit time fraction of wall: "
              << result.audit_time_fraction_of_wall << "\n";
    std::cout << "Diagnostics time fraction of wall: "
              << result.diagnostics_time_fraction_of_wall << "\n";
    std::cout << "Triangle BVH query time ms: "
              << result.diagnostics.triangle_bvh_query_time_ms << "\n";
    std::cout << "Box-triangle distance time ms: "
              << result.diagnostics.box_triangle_distance_time_ms << "\n";
    std::cout << "Marker refinement time ms: "
              << result.diagnostics.marker_refinement_time_ms << "\n";
    std::cout << "Distance refinement time ms: "
              << result.diagnostics.distance_refinement_time_ms << "\n";
    std::cout << "Contact-band max abs error: "
              << result.quality.contact_band_max_abs_error << "\n";
    std::cout << "Contact-band RMS error: "
              << result.quality.contact_band_rms_error << "\n";
    std::cout << "Contact-band P95 error: "
              << result.quality.contact_band_p95_error << "\n";
    std::cout << "Contact-band sign mismatches: "
              << result.quality.contact_band_sign_mismatch_count << "\n";
    std::cout << "Near-surface sign mismatches: "
              << result.quality.near_surface_sign_mismatch_count << "\n";
    std::cout << "Mean normal angle error deg: "
              << result.quality.mean_normal_angle_error_deg << "\n";
    std::cout << "P95 normal angle error deg: "
              << result.quality.p95_normal_angle_error_deg << "\n";
    std::cout << "Max normal angle error deg: "
              << result.quality.max_normal_angle_error_deg << "\n";
    std::cout << "Normal flips: " << result.quality.normal_flip_count << "\n";
    std::cout << "Near-surface normal flips: "
              << result.quality.near_surface_normal_flip_count << "\n";
    std::cout << "Coverage passed: "
              << (result.quality.coverage_passed ? "yes" : "no") << "\n";
    std::cout << "Coverage check count: "
              << result.quality.contact_band_coverage_check_count << "\n";
    std::cout << "Missed contact-band points: "
              << result.quality.missed_contact_band_point_count << "\n";
    std::cout << "Missed contact-band cells: "
              << result.quality.missed_contact_band_cell_count << "\n";
    std::cout << "Contact-band quality passed: "
              << (result.quality.contact_band_quality_passed ? "yes" : "no")
              << "\n";
    std::cout << "Effective speedup claim allowed: "
              << (result.effective_speedup_claim_allowed ? "yes" : "no")
              << "\n";
    std::cout << "Performance claim allowed: "
              << (result.performance_claim_allowed ? "yes" : "no")
              << "\n";
    std::cout << adasdf::ContactBandReportWriter::csvHeader() << "\n"
              << adasdf::ContactBandReportWriter::csvRow(result) << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_contact_band_sampling failed: "
              << exc.what() << "\n";
    return 3;
  }
}
