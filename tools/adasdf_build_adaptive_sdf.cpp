#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_adaptive_sdf input.stl output.sdfbin "
         "[--target-error 1e-3] [--surface-band-factor 1.5] "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--padding 0.05] [--signed|--unsigned] [--require-watertight] "
         "[--allow-open-unsigned] [--auto-clean] [--report build_report.md] "
         "[--json build_report.json] [--dry-run] [--enable-low-rank] "
         "[--accel brute|bvh] [--threads N] [--benchmark-brute-reference] "
         "[--target-compression-error 1e-3] [--max-rank N] "
         "[--fixed-rank N] [--keep-near-surface-dense] "
         "[--strict-json report.json] [--case-id case_id] "
         "[--recommend] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeReports(
    const adasdf::AdaptiveBlockSDFBuildReport& report,
    const std::filesystem::path& markdown,
    const std::filesystem::path& json) {
  if (!markdown.empty()) {
    adasdf::AdaptiveBlockSDFBuildReportWriter::writeMarkdown(
        markdown.string(),
        report);
  }
  if (!json.empty()) {
    adasdf::AdaptiveBlockSDFBuildReportWriter::writeJson(
        json.string(),
        report);
  }
}

adasdf::AdaptiveBlockSDFBuildReport dryRunReport(
    const adasdf::AdaptiveBlockSDFBuildOptions& options) {
  adasdf::AdaptiveBlockSDFBuildReport report;
  report.success = true;
  report.signed_distance = options.signed_distance;
  report.min_octree_level = options.min_octree_level;
  report.max_octree_level = options.max_octree_level;
  report.max_octree_level_used = options.max_octree_level;
  report.block_resolution = options.block_resolution;
  report.warnings.push_back(
      "dry-run only: no STL sampling and no .sdfbin output were written.");
  report.warnings.push_back(
      "Low-rank matrix-SVD block compression is available in v1.7.0-alpha.");
  return report;
}

bool parseAcceleration(
    const std::string& value,
    adasdf::AdaptiveBlockSDFBuildOptions* options) {
  adasdf::SDFSamplingAcceleration acceleration;
  if (!adasdf::parseSDFSamplingAcceleration(value, &acceleration)) {
    return false;
  }
  options->acceleration = acceleration;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
    bool dry_run = false;
    bool enable_low_rank = false;
    adasdf::AdaptiveBlockSDFBuildOptions options;
    adasdf::BlockLowRankCompressionOptions compression_options;
    const auto strict_timer = adasdf::startStrictRunTimer();
    std::map<std::string, std::string> strict_parameters =
        adasdf::commandLineParameters(argc, argv);
    auto write_strict =
        [&](bool success,
            const std::string& status,
            const std::string& failure_reason,
            const std::map<std::string, double>& metrics = {}) {
          if (strict_json_path.empty()) {
            return;
          }
          std::string strict_error;
          if (!adasdf::writeStrictRunReport(
                  strict_json_path,
                  "adasdf_build_adaptive_sdf",
                  case_id,
                  input,
                  output,
                  strict_parameters,
                  metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr << "adasdf_build_adaptive_sdf: failed to write strict JSON: "
                      << strict_error << "\n";
          }
        };

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--recommend") {
        std::cout
            << "Use adasdf_recommend_build for parameter recommendation.\n";
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.target_near_surface_error = std::stod(argv[++i]);
      } else if (arg == "--surface-band-factor" && hasValue(i, argc)) {
        options.surface_band_factor = std::stod(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        options.padding = std::stod(argv[++i]);
      } else if (arg == "--signed") {
        options.signed_distance = true;
      } else if (arg == "--unsigned") {
        options.signed_distance = false;
      } else if (arg == "--require-watertight") {
        options.require_watertight_for_signed = true;
      } else if (arg == "--allow-open-unsigned") {
        options.signed_distance = false;
        options.require_watertight_for_signed = false;
      } else if (arg == "--auto-clean") {
        options.auto_safe_cleanup = true;
      } else if (arg == "--accel" && hasValue(i, argc)) {
        if (!parseAcceleration(argv[++i], &options)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = std::stoi(argv[++i]);
      } else if (arg == "--benchmark-brute-reference") {
        options.benchmark_brute_reference = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (arg == "--dry-run") {
        dry_run = true;
      } else if (arg == "--verbose") {
        options.verbose = true;
      } else if (arg == "--enable-low-rank") {
        enable_low_rank = true;
      } else if (arg == "--target-compression-error" && hasValue(i, argc)) {
        const double value = std::stod(argv[++i]);
        compression_options.target_max_abs_error = value;
        compression_options.target_p95_error = value;
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        compression_options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--fixed-rank" && hasValue(i, argc)) {
        compression_options.fixed_rank = std::stoi(argv[++i]);
        compression_options.rank_selection = adasdf::RankSelectionMode::FixedRank;
      } else if (arg == "--keep-near-surface-dense") {
        compression_options.always_keep_near_surface_blocks_dense = true;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else if (output.empty()) {
        output = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty() || output.empty()) {
      usage();
      write_strict(false, "failed", "missing input or output path");
      return 1;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_adaptive_sdf: input STL does not exist: "
                << input.string() << "\n";
      write_strict(false, "failed", "input STL does not exist");
      return 1;
    }

    if (dry_run) {
      const adasdf::AdaptiveBlockSDFBuildReport report = dryRunReport(options);
      writeReports(report, report_path, json_path);
      std::cout << "AdaSDF-CL adaptive block SDF builder\n";
      std::cout << "Input: " << input.string() << "\n";
      std::cout << "Requested output: " << output.string() << "\n";
      std::cout << "Dry run: yes\n";
      std::cout << "Signed: " << (options.signed_distance ? "yes" : "no") << "\n";
      std::cout << "Octree levels: " << options.min_octree_level << "-"
                << options.max_octree_level << "\n";
      std::cout << "Block resolution: " << options.block_resolution << "\n";
      std::cout << "Acceleration: " << adasdf::toString(options.acceleration)
                << "\n";
      std::cout << "Threads requested: " << options.threads << "\n";
      std::cout << "Format: "
                << (enable_low_rank ? "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1"
                                    : "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")
                << "\n";
      std::cout << "Output written: no\n";
      std::cout << "Low-rank compression: "
                << (enable_low_rank ? "matrix-SVD enabled"
                                    : "not enabled")
                << "\n";
      if (!report_path.empty()) {
        std::cout << "Report: " << report_path.string() << "\n";
      }
      if (!json_path.empty()) {
        std::cout << "JSON report: " << json_path.string() << "\n";
      }
      write_strict(
          true,
          "ok",
          "",
          {{"build_time_ms", report.build_time_ms},
           {"memory_bytes", static_cast<double>(report.memory_bytes)}});
      return 0;
    }

    adasdf::AdaptiveBlockSDFBuildReport report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        input.string(),
        options,
        &report);
    writeReports(report, report_path, json_path);
    if (!model) {
      std::cerr << "adasdf_build_adaptive_sdf: build failed: "
                << report.error_message << "\n";
      write_strict(false, "failed", report.error_message);
      if (options.signed_distance && options.require_watertight_for_signed &&
          !report.watertight) {
        return 2;
      }
      if (report.error_message.find("Failed to read STL") != std::string::npos) {
        return 1;
      }
      return 3;
    }

    adasdf::BlockLowRankCompressionReport compression_report;
    std::shared_ptr<adasdf::SDFModel> output_model = model;
    if (enable_low_rank) {
      auto adaptive =
          std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
      if (!adaptive) {
        std::cerr << "adasdf_build_adaptive_sdf: low-rank compression requires "
                     "an AdaptiveBlockSDFModel.\n";
        write_strict(false, "failed", "low-rank compression requires AdaptiveBlockSDFModel");
        return 3;
      }
      adasdf::CompressedAdaptiveBlockSDF compressed =
          adasdf::BlockLowRankCompressor::compress(
              adaptive->blockSet(),
              compression_options,
              &compression_report);
      if (!compression_report.success) {
        std::cerr << "adasdf_build_adaptive_sdf: low-rank compression failed: "
                  << compression_report.error_message << "\n";
        write_strict(false, "failed", compression_report.error_message);
        return 3;
      }
      output_model =
          std::make_shared<adasdf::CompressedAdaptiveBlockSDFModel>(
              std::move(compressed));
    }

    try {
      adasdf::SDFBinWriter::write(output.string(), *output_model);
      auto reloaded = adasdf::SDFBinReader::read(output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded SDF model is invalid");
      }
    } catch (const std::exception& exc) {
      std::cerr
          << "adasdf_build_adaptive_sdf: write/reload validation failed: "
          << exc.what() << "\n";
      write_strict(false, "failed", exc.what());
      return 4;
    }

    std::cout << "AdaSDF-CL adaptive block SDF builder\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Signed: " << (report.signed_distance ? "yes" : "no") << "\n";
    std::cout << "Watertight: " << (report.watertight ? "yes" : "no") << "\n";
    std::cout << "Octree levels: " << report.min_octree_level << "-"
              << report.max_octree_level << "\n";
    std::cout << "Max level used: " << report.max_octree_level_used << "\n";
    std::cout << "Block resolution: " << report.block_resolution << "\n";
    std::cout << "Target near-surface error: "
              << options.target_near_surface_error << "\n";
    std::cout << "Octree nodes: " << report.octree_node_count << "\n";
    std::cout << "Leaf blocks: " << report.block_count << "\n";
    std::cout << "Near-surface blocks: "
              << report.near_surface_block_count << "\n";
    std::cout << "Memory bytes: " << report.memory_bytes << "\n";
    std::cout << "Acceleration: "
              << adasdf::toString(report.acceleration_stats.acceleration)
              << "\n";
    std::cout << "Used BVH: " << (report.used_bvh ? "yes" : "no") << "\n";
    std::cout << "Threads requested: "
              << report.acceleration_stats.threads_requested << "\n";
    std::cout << "Threads used: " << report.threads_used << "\n";
    std::cout << "BVH build time ms: "
              << report.acceleration_stats.bvh_build_time_ms << "\n";
    std::cout << "BVH nodes: "
              << report.acceleration_stats.bvh_node_count << "\n";
    std::cout << "BVH leaves: "
              << report.acceleration_stats.bvh_leaf_count << "\n";
    std::cout << "Brute reference time ms: "
              << report.acceleration_stats.brute_reference_time_ms << "\n";
    std::cout << "Speedup vs brute reference: "
              << report.acceleration_stats.speedup_vs_bruteforce << "\n";
    std::cout << "Sampling time ms: " << report.sampling_time_ms << "\n";
    std::cout << "Build time ms: " << report.build_time_ms << "\n";
    std::cout << "Format: "
              << (enable_low_rank ? "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1"
                                  : "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")
              << "\n";
    std::cout << "Reload validation: success\n";
    if (enable_low_rank) {
      std::cout << "Low-rank compression: matrix-SVD enabled\n";
      std::cout << "Matrix-SVD blocks: "
                << compression_report.compressed_block_count << "\n";
      std::cout << "Dense fallback blocks: "
                << compression_report.dense_fallback_block_count << "\n";
      std::cout << "Compression ratio: "
                << compression_report.compression_ratio << "\n";
      std::cout << "Tucker/HOSVD compression: planned / not implemented\n";
    } else {
      std::cout << "Low-rank compression: not enabled\n";
    }
    if (!report_path.empty()) {
      std::cout << "Report: " << report_path.string() << "\n";
    }
    if (!json_path.empty()) {
      std::cout << "JSON report: " << json_path.string() << "\n";
    }
    std::map<std::string, double> strict_metrics = {
        {"triangle_count",
         static_cast<double>(report.diagnostics.triangle_count)},
        {"vertex_count", static_cast<double>(report.diagnostics.vertex_count)},
        {"readiness_score", static_cast<double>(report.readiness.score)},
        {"build_time_ms", report.build_time_ms},
        {"memory_bytes", static_cast<double>(report.memory_bytes)}};
    if (enable_low_rank) {
      strict_metrics["compressed_memory_bytes"] =
          static_cast<double>(compression_report.compressed_memory_bytes);
      strict_metrics["compression_ratio"] =
          compression_report.compression_ratio;
      strict_metrics["max_abs_error"] =
          compression_report.global_max_abs_error;
      strict_metrics["rms_error"] = compression_report.global_rms_error;
      strict_metrics["p95_error"] = compression_report.global_p95_abs_error;
    }
    write_strict(true, "ok", "", strict_metrics);
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_adaptive_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
