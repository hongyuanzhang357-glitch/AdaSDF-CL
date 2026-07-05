#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
  std::string case_id = "contact_band";
  adasdf::ContactBandSamplingOptions contact;
};

struct Location {
  std::size_t block_index = 0;
  std::size_t local_index = 0;
};

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_contact_band_sampling input.stl "
         "[--max-level N] [--block-resolution N] [--target-error value] "
         "[--contact-band-width value] [--contact-band-layers N] "
         "[--halo-exact-layers N] [--far-field-resolution N] "
         "[--far-field-mode coarse-interpolate|constant-sign|clamped-distance] "
         "[--reuse-far-field-sign] [--no-reuse-far-field-sign] "
         "[--normal-audit] [--normal-error-limit-deg value] "
         "[--threads N] [--csv out.csv] [--report out.md] "
         "[--case-id id]\n";
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

adasdf::ContactBandBenchmarkResult runBenchmark(
    const adasdf::TriangleMesh& mesh,
    const Options& options) {
  adasdf::ContactBandBenchmarkResult result;
  result.case_id = options.case_id;

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

  std::vector<adasdf::ContactBandBlockSamplingResult> sampled(
      contact_blocks.size());
  const auto total0 = std::chrono::steady_clock::now();
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
  const auto total1 = std::chrono::steady_clock::now();
  result.contact_band_time_ms =
      std::chrono::duration<double, std::milli>(total1 - total0).count();

  adasdf::ContactBandDiagnostics diagnostics;
  adasdf::ContactBandQualityMetrics quality;
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
    diagnostics.exact_sampling_time_ms +=
        block_result.diagnostics.exact_sampling_time_ms;
    diagnostics.coarse_sampling_time_ms +=
        block_result.diagnostics.coarse_sampling_time_ms;
    diagnostics.interpolation_time_ms +=
        block_result.diagnostics.interpolation_time_ms;
    diagnostics.total_time_ms += block_result.diagnostics.total_time_ms;
    quality = adasdf::ContactBandQualityAudit::merge(
        quality,
        adasdf::ContactBandQualityAudit::auditBlock(
            contact_blocks[block_index],
            reference_blocks[block_index],
            block_result.mask,
            options.contact));
  }
  diagnostics.total_time_ms = result.contact_band_time_ms;
  adasdf::finalizeContactBandDiagnostics(&diagnostics);
  adasdf::ContactBandQualityAudit::finalize(&quality, options.contact);
  result.diagnostics = diagnostics;
  result.quality = quality;
  result.speedup =
      result.contact_band_time_ms > 0.0
          ? result.exact_reference_time_ms / result.contact_band_time_ms
          : 0.0;
  result.effective_speedup_claim_allowed =
      result.speedup > 1.0 && result.quality.contact_band_quality_passed;
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
      } else if (arg == "--reuse-far-field-sign") {
        options.contact.reuse_far_field_sign = true;
      } else if (arg == "--no-reuse-far-field-sign") {
        options.contact.reuse_far_field_sign = false;
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

    std::cout << "AdaSDF-CL contact-band sampling benchmark\n";
    std::cout << "Case id: " << result.case_id << "\n";
    std::cout << "Exact reference time ms: "
              << result.exact_reference_time_ms << "\n";
    std::cout << "Contact-band time ms: " << result.contact_band_time_ms
              << "\n";
    std::cout << "Speedup: " << result.speedup << "\n";
    std::cout << "Total blocks: "
              << result.diagnostics.total_block_count << "\n";
    std::cout << "Contact-band blocks: "
              << result.diagnostics.contact_band_block_count << "\n";
    std::cout << "Far-field blocks: "
              << result.diagnostics.far_field_block_count << "\n";
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
    std::cout << "Contact-band quality passed: "
              << (result.quality.contact_band_quality_passed ? "yes" : "no")
              << "\n";
    std::cout << "Effective speedup claim allowed: "
              << (result.effective_speedup_claim_allowed ? "yes" : "no")
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
