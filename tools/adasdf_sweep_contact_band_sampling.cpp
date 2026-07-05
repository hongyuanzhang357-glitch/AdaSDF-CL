#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct SweepCase {
  adasdf::ContactBandSamplingOptions contact;
  int halo_exact_layers = 1;
  int far_field_resolution = 3;
};

struct Options {
  std::filesystem::path input;
  int max_level = 4;
  int min_level = 0;
  int block_resolution = 8;
  double target_error = 1e-3;
  int threads = 1;
  bool signed_distance = true;
  bool normal_audit = false;
  double normal_limit_deg = 5.0;
  bool reuse_far_field_sign = true;
  std::filesystem::path csv;
  std::filesystem::path report;

  std::vector<adasdf::ContactBandMarkerMode> marker_modes = {
      adasdf::ContactBandMarkerMode::DistanceAware,
      adasdf::ContactBandMarkerMode::Hybrid};
  std::vector<double> contact_band_widths = {5e-4, 1e-3};
  std::vector<int> contact_band_layers = {1};
  std::vector<int> halo_exact_layers = {0, 1};
  std::vector<double> marker_cell_size_factors = {0.25, 0.5, 0.75, 1.0};
  std::vector<double> marker_safety_factors = {0.5, 1.0, 1.5};
  std::vector<bool> local_halo_only_values = {true};
  std::vector<int> far_field_resolutions = {2, 3};
  std::vector<adasdf::ContactBandFarFieldMode> far_field_modes = {
      adasdf::ContactBandFarFieldMode::CoarseInterpolate};
};

struct Location {
  std::size_t block_index = 0;
  std::size_t local_index = 0;
};

std::vector<std::string> splitComma(const std::string& value) {
  std::vector<std::string> out;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

std::vector<double> parseDoubleList(const std::string& value) {
  std::vector<double> out;
  for (const std::string& item : splitComma(value)) {
    out.push_back(std::stod(item));
  }
  return out;
}

std::vector<int> parseIntList(const std::string& value) {
  std::vector<int> out;
  for (const std::string& item : splitComma(value)) {
    out.push_back(std::stoi(item));
  }
  return out;
}

std::vector<bool> parseBoolList(const std::string& value) {
  std::vector<bool> out;
  for (const std::string& item : splitComma(value)) {
    out.push_back(item == "true" || item == "1" || item == "yes");
  }
  return out;
}

std::vector<adasdf::ContactBandMarkerMode> parseMarkerList(
    const std::string& value) {
  std::vector<adasdf::ContactBandMarkerMode> out;
  for (const std::string& item : splitComma(value)) {
    adasdf::ContactBandMarkerMode mode;
    if (!adasdf::parseContactBandMarkerMode(item, &mode)) {
      throw std::runtime_error("unknown contact-band marker: " + item);
    }
    out.push_back(mode);
  }
  return out;
}

std::vector<adasdf::ContactBandFarFieldMode> parseFarFieldList(
    const std::string& value) {
  std::vector<adasdf::ContactBandFarFieldMode> out;
  for (const std::string& item : splitComma(value)) {
    adasdf::ContactBandFarFieldMode mode;
    if (!adasdf::parseContactBandFarFieldMode(item, &mode)) {
      throw std::runtime_error("unknown far-field mode: " + item);
    }
    out.push_back(mode);
  }
  return out;
}

void usage() {
  std::cout
      << "Usage: adasdf_sweep_contact_band_sampling input.stl "
         "[--max-level N] [--block-resolution N] "
         "[--contact-band-marker conservative-aabb,distance-aware,hybrid] "
         "[--contact-band-width values] [--contact-band-layers values] "
         "[--halo-exact-layers values] "
         "[--marker-cell-size-factor values] "
         "[--marker-safety-factor values] [--local-halo-only true,false] "
         "[--far-field-resolution values] [--far-field-mode values] "
         "[--csv out.csv] [--report out.md]\n";
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

adasdf::ContactBandBenchmarkResult runCase(
    const adasdf::TriangleMesh& mesh,
    const Options& options,
    const adasdf::BVHSDFSampler& reference_sampler,
    adasdf::BVHSDFSampler& sampler,
    double exact_reference_time_ms,
    const std::vector<adasdf::AdaptiveSDFBlock>& reference_blocks,
    const adasdf::ContactBandSamplingOptions& contact,
    const std::string& case_id) {
  adasdf::ContactBandBenchmarkResult result;
  result.case_id = case_id;
  result.exact_reference_time_ms = exact_reference_time_ms;
  std::vector<adasdf::AdaptiveSDFBlock> contact_blocks = reference_blocks;
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
            reference_sampler.bvh(),
            contact);
      });
  const auto total1 = std::chrono::steady_clock::now();
  result.contact_band_time_ms =
      std::chrono::duration<double, std::milli>(total1 - total0).count();

  adasdf::ContactBandDiagnostics diagnostics;
  diagnostics.marker_mode = adasdf::toString(contact.marker_mode);
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
    diagnostics.marked_cell_count += block_result.diagnostics.marked_cell_count;
    diagnostics.marked_node_count += block_result.diagnostics.marked_node_count;
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
    diagnostics.marker_time_ms += block_result.diagnostics.marker_time_ms;
    diagnostics.distance_refinement_time_ms +=
        block_result.diagnostics.distance_refinement_time_ms;
    diagnostics.marker_refinement_time_ms +=
        block_result.diagnostics.marker_refinement_time_ms;
    diagnostics.box_triangle_distance_time_ms +=
        block_result.diagnostics.box_triangle_distance_time_ms;
    diagnostics.triangle_bvh_query_time_ms +=
        block_result.diagnostics.triangle_bvh_query_time_ms;
    quality = adasdf::ContactBandQualityAudit::merge(
        quality,
        adasdf::ContactBandQualityAudit::auditBlock(
            contact_blocks[block_index],
            reference_blocks[block_index],
            block_result.mask,
            contact));
  }
  const double accumulated_block_time_ms = diagnostics.total_time_ms;
  diagnostics.total_time_ms = result.contact_band_time_ms;
  adasdf::finalizeContactBandDiagnostics(&diagnostics);
  if (accumulated_block_time_ms > 0.0) {
    diagnostics.marker_time_fraction =
        diagnostics.marker_time_ms / accumulated_block_time_ms;
  }
  adasdf::ContactBandQualityAudit::finalize(&quality, contact);
  result.diagnostics = diagnostics;
  result.quality = quality;
  result.speedup =
      result.contact_band_time_ms > 0.0
          ? result.exact_reference_time_ms / result.contact_band_time_ms
          : 0.0;
  result.effective_speedup_including_marker = result.speedup;
  const double marker_fraction =
      std::max(0.0, std::min(1.0, result.diagnostics.marker_time_fraction));
  const double no_marker_time =
      result.contact_band_time_ms * (1.0 - marker_fraction);
  result.effective_speedup_excluding_marker =
      no_marker_time > 0.0
          ? result.exact_reference_time_ms / no_marker_time
          : 0.0;
  result.effective_speedup_claim_allowed =
      result.speedup > 1.0 && result.quality.contact_band_quality_passed;
  result.success = true;
  return result;
}

std::string caseId(std::size_t index, const adasdf::ContactBandSamplingOptions& c) {
  std::ostringstream out;
  out << "sweep_" << index << "_" << adasdf::toString(c.marker_mode)
      << "_w" << c.contact_band_width
      << "_h" << c.halo_exact_layers
      << "_f" << c.far_field_resolution
      << "_m" << c.marker_cell_size_factor
      << "_s" << c.marker_safety_factor
      << "_local" << (c.local_halo_only ? "1" : "0");
  return out.str();
}

std::string paramsCsv(const adasdf::ContactBandSamplingOptions& c) {
  std::ostringstream out;
  out << adasdf::toString(c.marker_mode) << ","
      << c.contact_band_width << "," << c.contact_band_layers << ","
      << c.halo_exact_layers << "," << c.marker_cell_size_factor << ","
      << c.marker_safety_factor << "," << (c.local_halo_only ? "true" : "false")
      << "," << c.far_field_resolution << ","
      << adasdf::toString(c.far_field_mode);
  return out.str();
}

const adasdf::ContactBandBenchmarkResult* bestSpeedup(
    const std::vector<adasdf::ContactBandBenchmarkResult>& results) {
  const adasdf::ContactBandBenchmarkResult* best = nullptr;
  for (const auto& result : results) {
    if (!result.quality.contact_band_quality_passed) {
      continue;
    }
    if (best == nullptr || result.speedup > best->speedup) {
      best = &result;
    }
  }
  return best;
}

const adasdf::ContactBandBenchmarkResult* bestExactRatio(
    const std::vector<adasdf::ContactBandBenchmarkResult>& results) {
  const adasdf::ContactBandBenchmarkResult* best = nullptr;
  for (const auto& result : results) {
    if (!result.quality.contact_band_quality_passed) {
      continue;
    }
    if (best == nullptr ||
        result.diagnostics.exact_node_ratio < best->diagnostics.exact_node_ratio) {
      best = &result;
    }
  }
  return best;
}

void writeCsv(
    const std::filesystem::path& path,
    const std::vector<adasdf::ContactBandBenchmarkResult>& results,
    const std::vector<adasdf::ContactBandSamplingOptions>& contacts) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path);
  out << "sweep_marker_mode,sweep_contact_band_width,"
         "sweep_contact_band_layers,sweep_halo_exact_layers,"
         "sweep_marker_cell_size_factor,sweep_marker_safety_factor,"
         "sweep_local_halo_only,sweep_far_field_resolution,"
         "sweep_far_field_mode,"
      << adasdf::ContactBandReportWriter::csvHeader() << "\n";
  for (std::size_t i = 0; i < results.size(); ++i) {
    out << paramsCsv(contacts[i]) << ","
        << adasdf::ContactBandReportWriter::csvRow(results[i]) << "\n";
  }
}

void writeReport(
    const std::filesystem::path& path,
    const std::vector<adasdf::ContactBandBenchmarkResult>& results,
    const std::vector<adasdf::ContactBandSamplingOptions>& contacts) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  const auto* best_speed = bestSpeedup(results);
  const auto* best_ratio = bestExactRatio(results);
  std::ofstream out(path);
  out << "# AdaSDF-CL Contact-Band Marker Sweep\n\n";
  out << "- Cases: " << results.size() << "\n";
  if (best_speed != nullptr) {
    out << "- Best speedup quality-pass case: " << best_speed->case_id
        << "\n";
    out << "- Best speedup: " << best_speed->speedup << "\n";
    out << "- Best speedup exact node ratio: "
        << best_speed->diagnostics.exact_node_ratio << "\n";
  } else {
    out << "- Best speedup quality-pass case: none\n";
  }
  if (best_ratio != nullptr) {
    out << "- Best exact-ratio quality-pass case: " << best_ratio->case_id
        << "\n";
    out << "- Best exact node ratio: "
        << best_ratio->diagnostics.exact_node_ratio << "\n";
  } else {
    out << "- Best exact-ratio quality-pass case: none\n";
  }
  out << "\n## Cases\n\n";
  out << "| Case | Params | Speedup | Exact ratio | Contact blocks | "
         "Far blocks | Quality | Effective speedup |\n";
  out << "| --- | --- | ---: | ---: | ---: | ---: | --- | --- |\n";
  for (std::size_t i = 0; i < results.size(); ++i) {
    const auto& result = results[i];
    out << "| " << result.case_id << " | " << paramsCsv(contacts[i])
        << " | " << result.speedup
        << " | " << result.diagnostics.exact_node_ratio
        << " | " << result.diagnostics.contact_band_block_count
        << " | " << result.diagnostics.far_field_block_count
        << " | "
        << (result.quality.contact_band_quality_passed ? "PASS" : "FAIL")
        << " | "
        << (result.effective_speedup_claim_allowed ? "yes" : "no")
        << " |\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options;
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
      } else if (arg == "--contact-band-marker" && hasValue(i, argc)) {
        options.marker_modes = parseMarkerList(argv[++i]);
      } else if (arg == "--contact-band-width" && hasValue(i, argc)) {
        options.contact_band_widths = parseDoubleList(argv[++i]);
      } else if (arg == "--contact-band-layers" && hasValue(i, argc)) {
        options.contact_band_layers = parseIntList(argv[++i]);
      } else if (arg == "--halo-exact-layers" && hasValue(i, argc)) {
        options.halo_exact_layers = parseIntList(argv[++i]);
      } else if (arg == "--marker-cell-size-factor" && hasValue(i, argc)) {
        options.marker_cell_size_factors = parseDoubleList(argv[++i]);
      } else if (arg == "--marker-safety-factor" && hasValue(i, argc)) {
        options.marker_safety_factors = parseDoubleList(argv[++i]);
      } else if (arg == "--local-halo-only" && hasValue(i, argc)) {
        options.local_halo_only_values = parseBoolList(argv[++i]);
      } else if (arg == "--far-field-resolution" && hasValue(i, argc)) {
        options.far_field_resolutions = parseIntList(argv[++i]);
      } else if (arg == "--far-field-mode" && hasValue(i, argc)) {
        options.far_field_modes = parseFarFieldList(argv[++i]);
      } else if (arg == "--reuse-far-field-sign") {
        options.reuse_far_field_sign = true;
      } else if (arg == "--no-reuse-far-field-sign") {
        options.reuse_far_field_sign = false;
      } else if (arg == "--normal-audit") {
        options.normal_audit = true;
      } else if (arg == "--normal-error-limit-deg" && hasValue(i, argc)) {
        options.normal_limit_deg = std::stod(argv[++i]);
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = std::stoi(argv[++i]);
      } else if (arg == "--csv" && hasValue(i, argc)) {
        options.csv = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        options.report = argv[++i];
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
      std::cerr << "adasdf_sweep_contact_band_sampling: input STL missing: "
                << options.input.string() << "\n";
      return 1;
    }
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(options.input.string());
    if (!read.success) {
      std::cerr << "adasdf_sweep_contact_band_sampling: failed to read STL: "
                << read.error_message << "\n";
      return 2;
    }

    adasdf::BVHSDFSamplerOptions sampler_options;
    sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    sampler_options.signed_distance = options.signed_distance;
    sampler_options.enable_counters = false;
    adasdf::BuildAccelerationStats acceleration_stats;
    acceleration_stats.threads_requested = std::max(1, options.threads);
    adasdf::BVHSDFSampler sampler;
    sampler.reset(read.mesh, sampler_options, &acceleration_stats);
    const adasdf::BVHSDFSampler& reference_sampler = sampler;
    std::vector<adasdf::AdaptiveSDFBlock> reference_blocks =
        makeBlocks(read.mesh, options);
    const double exact_reference_time_ms =
        sampleReferenceExact(
            read.mesh,
            reference_sampler,
            sampler_options,
            options.threads,
            &reference_blocks);

    std::vector<adasdf::ContactBandBenchmarkResult> results;
    std::vector<adasdf::ContactBandSamplingOptions> contacts;
    std::size_t case_index = 0;
    for (adasdf::ContactBandMarkerMode marker : options.marker_modes) {
      for (double width : options.contact_band_widths) {
        for (int layers : options.contact_band_layers) {
          for (int halo : options.halo_exact_layers) {
            for (double cell_factor : options.marker_cell_size_factors) {
              for (double safety : options.marker_safety_factors) {
                for (bool local_halo : options.local_halo_only_values) {
                  for (int far_res : options.far_field_resolutions) {
                    for (adasdf::ContactBandFarFieldMode far_mode :
                         options.far_field_modes) {
                      adasdf::ContactBandSamplingOptions contact;
                      contact.enable_contact_band_sampling = true;
                      contact.contact_band_width = width;
                      contact.contact_band_layers = layers;
                      contact.halo_exact_layers = halo;
                      contact.marker_mode = marker;
                      contact.marker_cell_size_factor = cell_factor;
                      contact.marker_safety_factor = safety;
                      contact.local_halo_only = local_halo;
                      contact.disable_global_halo = local_halo;
                      contact.far_field_resolution = far_res;
                      contact.far_field_mode = far_mode;
                      contact.reuse_far_field_sign =
                          options.reuse_far_field_sign;
                      contact.normal_audit = options.normal_audit;
                      contact.contact_band_normal_error_limit_deg =
                          options.normal_limit_deg;
                      const std::string id = caseId(case_index, contact);
                      results.push_back(
                          runCase(
                              read.mesh,
                              options,
                              reference_sampler,
                              sampler,
                              exact_reference_time_ms,
                              reference_blocks,
                              contact,
                              id));
                      contacts.push_back(contact);
                      ++case_index;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    writeCsv(options.csv, results, contacts);
    writeReport(options.report, results, contacts);

    const auto* best_speed = bestSpeedup(results);
    const auto* best_ratio = bestExactRatio(results);
    std::cout << "AdaSDF-CL contact-band marker sweep\n";
    std::cout << "Cases: " << results.size() << "\n";
    if (best_speed != nullptr) {
      std::cout << "Best speedup case: " << best_speed->case_id << "\n";
      std::cout << "Best speedup: " << best_speed->speedup << "\n";
      std::cout << "Best speedup exact node ratio: "
                << best_speed->diagnostics.exact_node_ratio << "\n";
    } else {
      std::cout << "Best speedup case: none\n";
    }
    if (best_ratio != nullptr) {
      std::cout << "Best exact node ratio case: " << best_ratio->case_id
                << "\n";
      std::cout << "Best exact node ratio: "
                << best_ratio->diagnostics.exact_node_ratio << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_sweep_contact_band_sampling failed: "
              << exc.what() << "\n";
    return 3;
  }
}
