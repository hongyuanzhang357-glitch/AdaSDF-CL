#include <adasdf/adasdf.h>

#include "BuildCliProfileHelpers.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_compress_adaptive_sdf input_adaptive.sdfbin "
         "output_compressed.sdfbin [--target-error 1e-3] [--target-rms 1e-4] "
         "[--target-p95 1e-3] [--fixed-rank N] [--min-rank N] "
         "[--max-rank N] [--memory-mb value] [--no-dense-fallback] "
         "[--keep-near-surface-dense] [--report compression_report.md] "
         "[--json compression_report.json] [--quality-report quality.md] "
         "[--strict-json report.json] [--case-id case_id] "
         "[--profile] [--profile-json profile.json] "
         "[--progress] [--progress-json progress.jsonl] "
         "[--max-seconds value] "
         "[--sample-cache off|block|global] "
         "[--corner-cache off|block|global] [--sign-cache off|on] "
         "[--distance-cache off|on] [--marker-cache off|on] "
         "[--cache-max-entries N] "
         "[--cache-quantization-epsilon value] [--report-cache-stats] "
         "[--distance-backend brute_force|brute|bvh] [--threads auto|N] "
         "[--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeReports(
    const adasdf::BlockLowRankCompressionReport& compression_report,
    const adasdf::CompressionQualityReport& quality_report,
    const std::filesystem::path& report_path,
    const std::filesystem::path& json_path,
    const std::filesystem::path& quality_report_path) {
  if (!report_path.empty()) {
    adasdf::CompressionReportWriter::writeMarkdown(
        report_path.string(),
        compression_report);
  }
  if (!json_path.empty()) {
    adasdf::CompressionReportWriter::writeJson(
        json_path.string(),
        compression_report);
  }
  if (!quality_report_path.empty()) {
    adasdf::CompressionReportWriter::writeQualityMarkdown(
        quality_report_path.string(),
        quality_report);
  }
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
    std::filesystem::path quality_report_path;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
    adasdf::BlockLowRankCompressionOptions options;
    adasdf_tools::BuildCliRuntimeOptions runtime_options;
    adasdf::BuildCacheOptions ignored_cache_options;
    bool ignored_cache_options_seen = false;
    std::string ignored_distance_backend;
    int ignored_threads = 0;
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
                  "adasdf_compress_adaptive_sdf",
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
            std::cerr << "adasdf_compress_adaptive_sdf: failed to write "
                         "strict JSON: "
                      << strict_error << "\n";
          }
        };

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      const auto common_result = adasdf_tools::parseBuildCliRuntimeOption(
          arg,
          &i,
          argc,
          argv,
          &runtime_options);
      if (common_result == adasdf_tools::CommonOptionParseResult::Error) {
        return 1;
      }
      if (common_result == adasdf_tools::CommonOptionParseResult::Parsed) {
        continue;
      }
      const auto cache_result = adasdf_tools::parseBuildCacheOption(
          arg,
          &i,
          argc,
          argv,
          &ignored_cache_options);
      if (cache_result == adasdf_tools::CommonOptionParseResult::Error) {
        return 1;
      }
      if (cache_result == adasdf_tools::CommonOptionParseResult::Parsed) {
        ignored_cache_options_seen = true;
        continue;
      }
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.target_max_abs_error = std::stod(argv[++i]);
      } else if (arg == "--target-rms" && hasValue(i, argc)) {
        options.target_rms_error = std::stod(argv[++i]);
      } else if (arg == "--target-p95" && hasValue(i, argc)) {
        options.target_p95_error = std::stod(argv[++i]);
      } else if (arg == "--fixed-rank" && hasValue(i, argc)) {
        options.fixed_rank = std::stoi(argv[++i]);
        options.rank_selection = adasdf::RankSelectionMode::FixedRank;
      } else if (arg == "--min-rank" && hasValue(i, argc)) {
        options.min_rank = std::stoi(argv[++i]);
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--memory-mb" && hasValue(i, argc)) {
        options.memory_budget_mb = std::stod(argv[++i]);
        options.rank_selection = adasdf::RankSelectionMode::MemoryBounded;
      } else if (arg == "--no-dense-fallback") {
        options.dense_fallback_if_error_exceeds_target = false;
      } else if (arg == "--keep-near-surface-dense") {
        options.always_keep_near_surface_blocks_dense = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--quality-report" && hasValue(i, argc)) {
        quality_report_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (arg == "--distance-backend" && hasValue(i, argc)) {
        adasdf::SDFSamplingAcceleration ignored_acceleration;
        ignored_distance_backend = argv[++i];
        if (!adasdf_tools::parseAccelerationAlias(
                ignored_distance_backend,
                &ignored_acceleration)) {
          std::cerr << "Unknown distance backend: " << ignored_distance_backend
                    << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        ignored_threads = adasdf_tools::parseThreadsAuto(argv[++i]);
      } else if (arg == "--verbose") {
        options.verbose = true;
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

    const bool profile_requested =
        adasdf_tools::profileRequested(runtime_options);
    adasdf::BuildProfiler profiler(profile_requested);
    profiler.setToolName("adasdf_compress_adaptive_sdf");
    profiler.setInputPath(input.string());
    profiler.setOutputPath(output.string());
    if (!ignored_distance_backend.empty()) {
      profiler.addWarning(
          "ADASDF_OPTION_IGNORED",
          "--distance-backend is accepted for script compatibility but is not "
          "used by the standalone compressor");
    }
    if (ignored_threads > 0) {
      profiler.addWarning(
          "ADASDF_OPTION_IGNORED",
          "--threads is accepted for script compatibility but is not used by "
          "the standalone compressor");
    }
    if (ignored_cache_options_seen) {
      profiler.addWarning(
          "ADASDF_OPTION_IGNORED",
          "build sample cache options are accepted for script compatibility "
          "but are not used by the standalone compressor");
    }
    adasdf::ProgressReporter progress(
        "adasdf_compress_adaptive_sdf",
        runtime_options.progress_stderr,
        runtime_options.progress_json_path);
    adasdf::TimeoutGuard timeout(runtime_options.max_seconds);
    auto write_profile = [&]() {
      adasdf_tools::writeBuildProfile(
          "adasdf_compress_adaptive_sdf",
          runtime_options,
          profiler);
    };
    auto timeout_exit = [&]() {
      write_strict(false, "timeout", "maximum wall-clock time reached");
      return adasdf_tools::timeoutExit(
          "adasdf_compress_adaptive_sdf",
          runtime_options,
          &profiler,
          progress);
    };
    progress.emit("load_mesh", "validating input adaptive SDF path", 0, 1);
    if (timeout.enabled() && runtime_options.max_seconds <= 0.01) {
      return timeout_exit();
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_compress_adaptive_sdf: input does not exist: "
                << input.string() << "\n";
      profiler.finishFailed(
          adasdf::ErrorCode::IO_ERROR,
          "input does not exist");
      write_profile();
      write_strict(false, "failed", "input does not exist");
      return 1;
    }

    std::shared_ptr<adasdf::SDFModel> model;
    {
      adasdf::BuildProfileScope scope(&profiler.timings.load_mesh_time_ms);
      model = adasdf::SDFBinReader::read(input);
    }
    auto adaptive = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
    if (!adaptive) {
      std::cerr
          << "adasdf_compress_adaptive_sdf: input must be "
             "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1 adaptive block SDF.\n";
      profiler.finishFailed(
          adasdf::ErrorCode::INVALID_MODEL,
          "input must be adaptive block SDF");
      write_profile();
      write_strict(false, "failed", "input must be adaptive block SDF");
      return 2;
    }
    if (timeout.expired()) {
      return timeout_exit();
    }

    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed;
    {
      adasdf::BuildProfileScope scope(&profiler.timings.compression_time_ms);
      progress.emit("compression", "compressing adaptive blocks", 0, 1);
      compressed = adasdf::BlockLowRankCompressor::compress(
          adaptive->blockSet(),
          options,
          &compression_report);
    }
    adasdf_tools::fillCompressionProfile(&profiler, compression_report);
    if (!compression_report.success) {
      std::cerr << "adasdf_compress_adaptive_sdf: compression failed: "
                << compression_report.error_message << "\n";
      profiler.finishFailed(
          adasdf::ErrorCode::INTERNAL_ERROR,
          compression_report.error_message);
      write_profile();
      write_strict(false, "failed", compression_report.error_message);
      return 3;
    }
    if (timeout.expired()) {
      return timeout_exit();
    }

    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    if (!compressed_model.isValid()) {
      std::cerr << "adasdf_compress_adaptive_sdf: compressed model is invalid.\n";
      profiler.finishFailed(
          adasdf::ErrorCode::INTERNAL_ERROR,
          "compressed model is invalid");
      write_profile();
      write_strict(false, "failed", "compressed model is invalid");
      return 3;
    }

    adasdf::CompressionQualityReport quality_report;
    {
      adasdf::BuildProfileScope scope(&profiler.timings.metadata_time_ms);
      quality_report = adasdf::CompressionQuality::compare(
          *adaptive,
          compressed_model);
    }
    writeReports(
        compression_report,
        quality_report,
        report_path,
        json_path,
        quality_report_path);

    try {
      adasdf::BuildProfileScope scope(&profiler.timings.write_model_time_ms);
      progress.emit("write_model", "writing compressed SDF model", 0, 1);
      std::filesystem::path temp_output = output;
      temp_output += ".tmp";
      adasdf::SDFBinWriter::write(temp_output.string(), compressed_model);
      auto reloaded = adasdf::SDFBinReader::read(temp_output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded compressed model is invalid");
      }
      if (std::filesystem::exists(output)) {
        std::filesystem::remove(output);
      }
      std::filesystem::rename(temp_output, output);
      profiler.counters.output_bytes = std::filesystem::file_size(output);
    } catch (const std::exception& exc) {
      std::cerr
          << "adasdf_compress_adaptive_sdf: write/reload validation failed: "
          << exc.what() << "\n";
      profiler.finishFailed(adasdf::ErrorCode::IO_ERROR, exc.what());
      write_profile();
      write_strict(false, "failed", exc.what());
      return 4;
    }

    if (!quality_report.success) {
      std::cerr << "adasdf_compress_adaptive_sdf: quality check failed: "
                << quality_report.error_message << "\n";
      profiler.finishFailed(
          adasdf::ErrorCode::INTERNAL_ERROR,
          quality_report.error_message);
      write_profile();
      write_strict(false, "failed", quality_report.error_message);
      return 5;
    }
    progress.emit("complete", "adaptive SDF compression complete", 1, 1);
    profiler.finishCompleted();

    std::cout << "AdaSDF-CL adaptive block SDF compressor\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1\n";
    std::cout << "Matrix-SVD blocks: "
              << compression_report.compressed_block_count << "\n";
    std::cout << "Dense fallback blocks: "
              << compression_report.dense_fallback_block_count << "\n";
    std::cout << "Original memory bytes: "
              << compression_report.original_memory_bytes << "\n";
    std::cout << "Compressed memory bytes: "
              << compression_report.compressed_memory_bytes << "\n";
    std::cout << "Compression ratio: "
              << compression_report.compression_ratio << "\n";
    std::cout << "Max abs error: "
              << compression_report.global_max_abs_error << "\n";
    std::cout << "RMS error: " << compression_report.global_rms_error << "\n";
    std::cout << "P95 abs error: "
              << compression_report.global_p95_abs_error << "\n";
    std::cout << "Quality samples: " << quality_report.sample_count << "\n";
    std::cout << "Reload validation: success\n";
    std::cout << "Tucker/HOSVD compression: planned / not implemented\n";
    std::cout << "GPU-native compressed query: planned\n";
    if (!report_path.empty()) {
      std::cout << "Report: " << report_path.string() << "\n";
    }
    if (!json_path.empty()) {
      std::cout << "JSON report: " << json_path.string() << "\n";
    }
    if (!quality_report_path.empty()) {
      std::cout << "Quality report: " << quality_report_path.string() << "\n";
    }
    write_strict(
        true,
        "ok",
        "",
        {{"memory_bytes",
          static_cast<double>(compression_report.original_memory_bytes)},
         {"compressed_memory_bytes",
          static_cast<double>(compression_report.compressed_memory_bytes)},
         {"compression_ratio", compression_report.compression_ratio},
         {"max_abs_error", compression_report.global_max_abs_error},
         {"mean_abs_error", compression_report.global_mean_abs_error},
         {"rms_error", compression_report.global_rms_error},
         {"p95_error", compression_report.global_p95_abs_error},
         {"sign_mismatch_count",
          static_cast<double>(compression_report.sign_mismatch_count)},
         {"near_surface_sign_mismatch_count",
          static_cast<double>(
              compression_report.near_surface_sign_mismatch_count)}});
    write_profile();
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_compress_adaptive_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
