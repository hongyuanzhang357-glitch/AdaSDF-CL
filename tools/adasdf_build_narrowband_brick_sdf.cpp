#include <adasdf/adasdf.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct CliOptions {
  std::filesystem::path input_stl;
  std::filesystem::path output_sdf;
  adasdf::NarrowBandBrickBuildOptions build;
  std::filesystem::path profile_json;
  std::filesystem::path progress_json;
  std::filesystem::path report;
  std::filesystem::path planning_json;
  std::filesystem::path planning_report;
};

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

void usage() {
  std::cerr
      << "Usage: adasdf_build_narrowband_brick_sdf input.stl output.sdfbin "
         "[--max-sampling-level N] [--brick-level-map 0-2:0,3-5:1] "
         "[--sampling-levels-per-brick-level N] "
         "[--brick-min-tensor-dim N] [--brick-target-tensor-dim N] "
         "[--brick-max-tensor-dim N] [--brick-max-expanded-kb N] "
         "[--brick-split-on-memory] [--brick-split-on-curvature] "
         "[--brick-split-on-rank-risk] [--brick-merge-small] "
         "[--sampling-contact-band-width value] "
         "[--sampling-mode preview|collision|high-quality] "
         "[--sampling-refine-zero-crossing] "
         "[--sampling-refine-contact-band] "
         "[--sampling-refine-curvature-hint] "
         "[--sampling-refine-small-gap-hint] "
         "[--tensor-fill exact-all|contact-exact-far-interp|coarse-prolongation] "
         "[--compression-mode weighted-low-rank|dense|existing-low-rank] "
         "[--near-zero-sign-guard] [--normal-guard] "
         "[--dense-fallback-on-sign-flip] [--max-rank N] "
         "[--contact-exact-spacing value] "
         "[--contact-exact-min-node-ratio value] "
         "[--contact-exact-stencil 1|2|3] "
         "[--zero-crossing-exact-stencil N] "
         "[--contact-exact-from-surface-samples N] "
         "[--contact-exact-from-zero-crossing-cells] "
         "[--contact-exact-normal-offsets list] "
         "[--sign-protected-fill] "
         "[--zero-crossing-cell-fill exact-stencil|subgrid2|subgrid4|linear] "
         "[--fill-sign-check] [--fill-sign-fallback exact|linear] "
         "[--query-backend auto|legacy|brick-fast] "
         "[--threads auto|N] [--max-seconds N] "
         "[--profile-json path] [--progress-json path] "
         "[--planning-json path] [--planning-report path] [--report path]\n";
}

bool needValue(int argc, int index, const std::string& arg) {
  if (index + 1 < argc) {
    return true;
  }
  std::cerr << arg << " requires a value\n";
  return false;
}

std::vector<double> parseDoubleList(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stod(item));
    }
  }
  return values;
}

bool parseArgs(int argc, char** argv, CliOptions* options) {
  if (argc < 3 || options == nullptr) {
    return false;
  }
  options->input_stl = argv[1];
  options->output_sdf = argv[2];
  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--max-sampling-level" && needValue(argc, i, arg)) {
      options->build.max_sampling_level = std::stoi(argv[++i]);
    } else if (arg == "--brick-level-map" && needValue(argc, i, arg)) {
      std::string error;
      if (!adasdf::parseBrickLevelMap(
              argv[++i],
              &options->build.brick_level_map,
              &error)) {
        std::cerr << error << "\n";
        return false;
      }
    } else if (arg == "--sampling-levels-per-brick-level" &&
               needValue(argc, i, arg)) {
      options->build.sampling_levels_per_brick_level = std::stoi(argv[++i]);
    } else if (arg == "--brick-min-tensor-dim" && needValue(argc, i, arg)) {
      options->build.brick_min_tensor_dim = std::stoi(argv[++i]);
    } else if (arg == "--brick-target-tensor-dim" && needValue(argc, i, arg)) {
      options->build.brick_target_tensor_dim = std::stoi(argv[++i]);
    } else if (arg == "--brick-max-tensor-dim" && needValue(argc, i, arg)) {
      options->build.brick_max_tensor_dim = std::stoi(argv[++i]);
    } else if (arg == "--brick-max-expanded-kb" && needValue(argc, i, arg)) {
      options->build.brick_max_expanded_kb = std::stoi(argv[++i]);
    } else if (arg == "--brick-min-compression-nodes" &&
               needValue(argc, i, arg)) {
      options->build.brick_min_compression_nodes = std::stoi(argv[++i]);
    } else if (arg == "--brick-split-on-high-rank") {
      options->build.brick_split_on_high_rank = true;
    } else if (arg == "--brick-split-on-rank-risk") {
      options->build.brick_split_on_high_rank = true;
    } else if (arg == "--brick-split-on-curvature") {
      options->build.brick_split_on_curvature = true;
    } else if (arg == "--brick-split-on-memory") {
      options->build.brick_split_on_memory = true;
    } else if (arg == "--brick-merge-small") {
      options->build.brick_merge_small = true;
    } else if (arg == "--sampling-contact-band-width" &&
               needValue(argc, i, arg)) {
      options->build.sampling_contact_band_width = std::stod(argv[++i]);
    } else if (arg == "--sampling-refine-zero-crossing") {
      options->build.sampling_refine_zero_crossing = true;
    } else if (arg == "--sampling-refine-contact-band") {
      options->build.sampling_refine_contact_band = true;
    } else if (arg == "--sampling-refine-curvature-hint") {
      options->build.sampling_refine_curvature_hint = true;
    } else if (arg == "--sampling-refine-small-gap-hint") {
      options->build.sampling_refine_small_gap_hint = true;
    } else if (arg == "--sampling-curvature-aware") {
      options->build.sampling_curvature_aware = true;
    } else if (arg == "--sampling-small-gap-aware") {
      options->build.sampling_small_gap_aware = true;
    } else if (arg == "--sampling-mode" && needValue(argc, i, arg)) {
      options->build.sampling_mode = argv[++i];
    } else if (arg == "--tensor-fill" && needValue(argc, i, arg)) {
      adasdf::NarrowBandTensorFillMode mode;
      if (!adasdf::parseNarrowBandTensorFillMode(argv[++i], &mode)) {
        std::cerr << "invalid tensor-fill\n";
        return false;
      }
      options->build.tensor_fill = mode;
    } else if (arg == "--compression-mode" && needValue(argc, i, arg)) {
      adasdf::NarrowBandCompressionMode mode;
      if (!adasdf::parseNarrowBandCompressionMode(argv[++i], &mode)) {
        std::cerr << "invalid compression-mode\n";
        return false;
      }
      options->build.compression_mode = mode;
    } else if (arg == "--contact-weight" && needValue(argc, i, arg)) {
      options->build.contact_weight = std::stod(argv[++i]);
    } else if (arg == "--far-field-weight" && needValue(argc, i, arg)) {
      options->build.far_field_weight = std::stod(argv[++i]);
    } else if (arg == "--near-zero-sign-guard") {
      options->build.near_zero_sign_guard = true;
    } else if (arg == "--normal-guard") {
      options->build.normal_guard = true;
    } else if (arg == "--dense-fallback-on-sign-flip") {
      options->build.dense_fallback_on_sign_flip = true;
    } else if (arg == "--dense-fallback-on-contact-error") {
      options->build.dense_fallback_on_contact_error = true;
    } else if (arg == "--max-rank" && needValue(argc, i, arg)) {
      options->build.max_rank = std::stoi(argv[++i]);
    } else if (arg == "--rank-auto") {
      options->build.rank_auto = true;
    } else if (arg == "--contact-exact-spacing" &&
               needValue(argc, i, arg)) {
      options->build.contact_exact_spacing = std::stod(argv[++i]);
    } else if (arg == "--contact-exact-min-node-ratio" &&
               needValue(argc, i, arg)) {
      options->build.contact_exact_min_node_ratio = std::stod(argv[++i]);
    } else if (arg == "--contact-exact-stencil" &&
               needValue(argc, i, arg)) {
      options->build.contact_exact_stencil = std::stoi(argv[++i]);
    } else if (arg == "--zero-crossing-exact-stencil" &&
               needValue(argc, i, arg)) {
      options->build.zero_crossing_exact_stencil = std::stoi(argv[++i]);
    } else if (arg == "--contact-exact-from-surface-samples" &&
               needValue(argc, i, arg)) {
      options->build.contact_exact_from_surface_samples = std::stoi(argv[++i]);
    } else if (arg == "--contact-exact-from-zero-crossing-cells") {
      options->build.contact_exact_from_zero_crossing_cells = true;
    } else if (arg == "--contact-exact-normal-offsets" &&
               needValue(argc, i, arg)) {
      options->build.contact_exact_normal_offsets = parseDoubleList(argv[++i]);
    } else if (arg == "--sign-protected-fill") {
      options->build.sign_protected_fill = true;
    } else if (arg == "--zero-crossing-cell-fill" &&
               needValue(argc, i, arg)) {
      options->build.zero_crossing_cell_fill = argv[++i];
    } else if (arg == "--fill-sign-check") {
      options->build.fill_sign_check = true;
    } else if (arg == "--fill-sign-fallback" && needValue(argc, i, arg)) {
      options->build.fill_sign_fallback = argv[++i];
    } else if (arg == "--query-backend" && needValue(argc, i, arg)) {
      options->build.query_backend = argv[++i];
    } else if (arg == "--threads" && needValue(argc, i, arg)) {
      const std::string value = argv[++i];
      options->build.thread_count = value == "auto" ? 0 : std::stoi(value);
    } else if (arg == "--max-seconds" && needValue(argc, i, arg)) {
      options->build.max_seconds = std::stod(argv[++i]);
    } else if (arg == "--profile-json" && needValue(argc, i, arg)) {
      options->profile_json = argv[++i];
    } else if (arg == "--progress-json" && needValue(argc, i, arg)) {
      options->progress_json = argv[++i];
    } else if (arg == "--planning-json" && needValue(argc, i, arg)) {
      options->planning_json = argv[++i];
    } else if (arg == "--planning-report" && needValue(argc, i, arg)) {
      options->planning_report = argv[++i];
    } else if (arg == "--report" && needValue(argc, i, arg)) {
      options->report = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    }
  }
  if (options->build.brick_level_map.empty()) {
    std::string error;
    if (!adasdf::parseBrickLevelMap(
            "0-2:0,3-5:1",
            &options->build.brick_level_map,
            &error)) {
      std::cerr << error << "\n";
      return false;
    }
  }
  return true;
}

bool validate(const CliOptions& options) {
  if (options.input_stl.empty() || options.output_sdf.empty()) {
    std::cerr << "input STL and output SDF are required\n";
    return false;
  }
  if (!std::filesystem::exists(options.input_stl)) {
    std::cerr << "input STL not found: " << options.input_stl.string()
              << "\n";
    return false;
  }
  if (options.build.max_sampling_level < 0 ||
      options.build.brick_min_tensor_dim < 2 ||
      options.build.brick_target_tensor_dim < 2 ||
      options.build.brick_max_tensor_dim < 2 ||
      options.build.brick_min_tensor_dim >
          options.build.brick_max_tensor_dim ||
      options.build.contact_exact_min_node_ratio < 0.0 ||
      options.build.contact_exact_min_node_ratio > 1.0 ||
      options.build.contact_exact_stencil < 1 ||
      options.build.zero_crossing_exact_stencil < 1 ||
      !(options.build.sampling_contact_band_width > 0.0)) {
    std::cerr << "invalid narrow-band brick build options\n";
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    CliOptions cli;
    if (!parseArgs(argc, argv, &cli) || !validate(cli)) {
      usage();
      return 2;
    }

    adasdf::NarrowBandBrickBuildStats stats;
    const Clock::time_point total_start = Clock::now();
    const Clock::time_point mesh_start = Clock::now();
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(cli.input_stl.string());
    stats.mesh_load_time_ms = elapsedMs(mesh_start, Clock::now());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }

    std::shared_ptr<adasdf::AdaptiveBlockSDFModel> model =
        adasdf::NarrowBandBrickSDFBuilder::fromMesh(
            read.mesh,
            cli.build,
            &stats);
    const Clock::time_point write_start = Clock::now();
    adasdf::SDFBinWriter::write(cli.output_sdf.string(), *model);
    stats.write_time_ms = elapsedMs(write_start, Clock::now());
    stats.total_time_ms = elapsedMs(total_start, Clock::now());

    if (!cli.profile_json.empty()) {
      adasdf::NarrowBandBrickReportWriter::writeJson(
          cli.profile_json,
          stats,
          cli.build);
    }
    if (!cli.planning_json.empty()) {
      adasdf::NarrowBandBrickReportWriter::writeJson(
          cli.planning_json,
          stats,
          cli.build);
    }
    if (!cli.progress_json.empty()) {
      adasdf::NarrowBandBrickReportWriter::writeProgressJsonl(
          cli.progress_json,
          stats);
    }
    if (!cli.report.empty()) {
      adasdf::NarrowBandBrickReportWriter::writeMarkdown(
          cli.report,
          stats,
          cli.build);
    }
    if (!cli.planning_report.empty()) {
      adasdf::NarrowBandBrickReportWriter::writeMarkdown(
          cli.planning_report,
          stats,
          cli.build);
    }

    std::cout << "AdaSDF-CL narrow-band brick SDF builder\n";
    std::cout << "Input: " << cli.input_stl.string() << "\n";
    std::cout << "Output: " << cli.output_sdf.string() << "\n";
    std::cout << "Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1\n";
    std::cout << "Sampling nodes: " << stats.sampling_node_count << "\n";
    std::cout << "Brick count: " << stats.brick_count << "\n";
    std::cout << "Total tensor nodes: " << stats.total_tensor_nodes << "\n";
    std::cout << "Exact source nodes: "
              << stats.total_exact_source_nodes << "\n";
    std::cout << "Interpolated fill nodes: "
              << stats.total_interpolated_fill_nodes << "\n";
    std::cout << "Dense fallback blocks: "
              << stats.dense_fallback_block_count << "\n";
    std::cout << "Compressed blocks: "
              << stats.compressed_block_count << "\n";
    std::cout << "Estimated expanded bytes: "
              << stats.estimated_expanded_bytes << "\n";
    std::cout << "Build time ms: " << stats.total_time_ms << "\n";
    if (!cli.report.empty()) {
      std::cout << "Report: " << cli.report.string() << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_narrowband_brick_sdf failed: "
              << exc.what() << "\n";
    return 1;
  }
}
