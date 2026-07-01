#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_compressed_sdf input.stl output_compressed.sdfbin "
         "[--target-error 1e-3] [--max-level N] [--min-level N] "
         "[--block-resolution N] [--padding 0.05] [--signed|--unsigned] "
         "[--auto-clean] [--fixed-rank N] [--min-rank N] [--max-rank N] "
         "[--keep-near-surface-dense] [--report build_report.md] "
         "[--compression-report compression_report.md] "
         "[--quality-report quality_report.md] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
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
    std::filesystem::path build_report_path;
    std::filesystem::path compression_report_path;
    std::filesystem::path quality_report_path;
    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    adasdf::BlockLowRankCompressionOptions compression_options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        const double value = std::stod(argv[++i]);
        build_options.target_near_surface_error = value;
        compression_options.target_max_abs_error = value;
        compression_options.target_p95_error = value;
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        build_options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        build_options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        build_options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        build_options.padding = std::stod(argv[++i]);
      } else if (arg == "--signed") {
        build_options.signed_distance = true;
      } else if (arg == "--unsigned") {
        build_options.signed_distance = false;
        build_options.require_watertight_for_signed = false;
      } else if (arg == "--auto-clean") {
        build_options.auto_safe_cleanup = true;
      } else if (arg == "--fixed-rank" && hasValue(i, argc)) {
        compression_options.fixed_rank = std::stoi(argv[++i]);
        compression_options.rank_selection = adasdf::RankSelectionMode::FixedRank;
      } else if (arg == "--min-rank" && hasValue(i, argc)) {
        compression_options.min_rank = std::stoi(argv[++i]);
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        compression_options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--keep-near-surface-dense") {
        compression_options.always_keep_near_surface_blocks_dense = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        build_report_path = argv[++i];
      } else if (arg == "--compression-report" && hasValue(i, argc)) {
        compression_report_path = argv[++i];
      } else if (arg == "--quality-report" && hasValue(i, argc)) {
        quality_report_path = argv[++i];
      } else if (arg == "--verbose") {
        build_options.verbose = true;
        compression_options.verbose = true;
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
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_compressed_sdf: input STL does not exist: "
                << input.string() << "\n";
      return 1;
    }

    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto dense_model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        input.string(),
        build_options,
        &build_report);
    if (!build_report_path.empty()) {
      adasdf::AdaptiveBlockSDFBuildReportWriter::writeMarkdown(
          build_report_path.string(),
          build_report);
    }
    if (!dense_model) {
      std::cerr << "adasdf_build_compressed_sdf: adaptive build failed: "
                << build_report.error_message << "\n";
      return build_options.signed_distance &&
                     build_options.require_watertight_for_signed &&
                     !build_report.watertight
                 ? 2
                 : 3;
    }

    auto adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(dense_model);
    if (!adaptive) {
      std::cerr << "adasdf_build_compressed_sdf: adaptive builder returned "
                   "unexpected model type.\n";
      return 3;
    }

    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            compression_options,
            &compression_report);
    if (!compression_report_path.empty()) {
      adasdf::CompressionReportWriter::writeMarkdown(
          compression_report_path.string(),
          compression_report);
    }
    if (!compression_report.success) {
      std::cerr << "adasdf_build_compressed_sdf: compression failed: "
                << compression_report.error_message << "\n";
      return 3;
    }

    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    adasdf::CompressionQualityReport quality_report =
        adasdf::CompressionQuality::compare(*adaptive, compressed_model);
    if (!quality_report_path.empty()) {
      adasdf::CompressionReportWriter::writeQualityMarkdown(
          quality_report_path.string(),
          quality_report);
    }

    try {
      adasdf::SDFBinWriter::write(output.string(), compressed_model);
      auto reloaded = adasdf::SDFBinReader::read(output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded compressed model is invalid");
      }
    } catch (const std::exception& exc) {
      std::cerr
          << "adasdf_build_compressed_sdf: write/reload validation failed: "
          << exc.what() << "\n";
      return 4;
    }

    if (!quality_report.success) {
      std::cerr << "adasdf_build_compressed_sdf: quality check failed: "
                << quality_report.error_message << "\n";
      return 5;
    }

    std::cout << "AdaSDF-CL compressed SDF builder\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1\n";
    std::cout << "Adaptive blocks: " << build_report.block_count << "\n";
    std::cout << "Matrix-SVD blocks: "
              << compression_report.compressed_block_count << "\n";
    std::cout << "Dense fallback blocks: "
              << compression_report.dense_fallback_block_count << "\n";
    std::cout << "Compression ratio: "
              << compression_report.compression_ratio << "\n";
    std::cout << "Max abs error: "
              << compression_report.global_max_abs_error << "\n";
    std::cout << "Quality samples: " << quality_report.sample_count << "\n";
    std::cout << "Reload validation: success\n";
    std::cout << "Tucker/HOSVD compression: planned / not implemented\n";
    std::cout << "Surrogate recommendation: planned for v1.8.0-alpha\n";
    std::cout << "GPU-native compressed query: planned\n";
    if (!build_report_path.empty()) {
      std::cout << "Build report: " << build_report_path.string() << "\n";
    }
    if (!compression_report_path.empty()) {
      std::cout << "Compression report: "
                << compression_report_path.string() << "\n";
    }
    if (!quality_report_path.empty()) {
      std::cout << "Quality report: " << quality_report_path.string() << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_compressed_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
