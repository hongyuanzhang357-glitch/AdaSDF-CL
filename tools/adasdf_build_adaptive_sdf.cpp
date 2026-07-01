#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_adaptive_sdf input.stl output.sdfbin "
         "[--target-error 1e-3] [--surface-band-factor 1.5] "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--padding 0.05] [--signed|--unsigned] [--require-watertight] "
         "[--allow-open-unsigned] [--auto-clean] [--report build_report.md] "
         "[--json build_report.json] [--dry-run] [--verbose]\n";
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
      "Low-rank compression is planned for v1.7.0-alpha and is not "
      "implemented in v1.6.0-alpha.");
  return report;
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
    bool dry_run = false;
    bool enable_low_rank = false;
    adasdf::AdaptiveBlockSDFBuildOptions options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
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
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--dry-run") {
        dry_run = true;
      } else if (arg == "--verbose") {
        options.verbose = true;
      } else if (arg == "--enable-low-rank") {
        enable_low_rank = true;
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
      return 1;
    }
    if (enable_low_rank) {
      std::cerr
          << "Low-rank compression is planned for v1.7.0-alpha and is not "
             "implemented in v1.6.0-alpha.\n";
      return 5;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_adaptive_sdf: input STL does not exist: "
                << input.string() << "\n";
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
      std::cout << "Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1\n";
      std::cout << "Output written: no\n";
      std::cout
          << "Low-rank compression: planned for v1.7.0-alpha, not "
             "implemented in v1.6.0-alpha\n";
      if (!report_path.empty()) {
        std::cout << "Report: " << report_path.string() << "\n";
      }
      if (!json_path.empty()) {
        std::cout << "JSON report: " << json_path.string() << "\n";
      }
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
      if (options.signed_distance && options.require_watertight_for_signed &&
          !report.watertight) {
        return 2;
      }
      if (report.error_message.find("Failed to read STL") != std::string::npos) {
        return 1;
      }
      return 3;
    }

    try {
      adasdf::SDFBinWriter::write(output.string(), *model);
      auto reloaded = adasdf::SDFBinReader::read(output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded AdaptiveBlockSDF model is invalid");
      }
    } catch (const std::exception& exc) {
      std::cerr
          << "adasdf_build_adaptive_sdf: write/reload validation failed: "
          << exc.what() << "\n";
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
    std::cout << "Sampling time ms: " << report.sampling_time_ms << "\n";
    std::cout << "Build time ms: " << report.build_time_ms << "\n";
    std::cout << "Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1\n";
    std::cout << "Reload validation: success\n";
    std::cout
        << "Low-rank compression: not enabled / planned for v1.7.0-alpha\n";
    if (!report_path.empty()) {
      std::cout << "Report: " << report_path.string() << "\n";
    }
    if (!json_path.empty()) {
      std::cout << "JSON report: " << json_path.string() << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_adaptive_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
