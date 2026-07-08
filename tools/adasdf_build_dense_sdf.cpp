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
      << "Usage: adasdf_build_dense_sdf input.stl output.sdfbin "
         "[--resolution 64] [--padding 0.05] [--signed|--unsigned] "
         "[--require-watertight] [--allow-open-unsigned] [--auto-clean] "
         "[--accel brute|bvh] [--distance-backend brute_force|brute|bvh] "
         "[--threads auto|N] [--benchmark-brute-reference] "
         "[--sampling exact|hierarchical] "
         "[--report build_report.md] [--json build_report.json] "
         "[--strict-json report.json] [--case-id case_id] "
         "[--profile] [--profile-json profile.json] "
         "[--progress] [--progress-json progress.jsonl] "
         "[--max-seconds value] "
         "[--recommend] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeReports(
    const adasdf::DenseSDFBuildReport& report,
    const std::filesystem::path& markdown,
    const std::filesystem::path& json) {
  if (!markdown.empty()) {
    adasdf::DenseSDFBuildReportWriter::writeMarkdown(markdown.string(), report);
  }
  if (!json.empty()) {
    adasdf::DenseSDFBuildReportWriter::writeJson(json.string(), report);
  }
}

bool parseAcceleration(const std::string& value, adasdf::DenseSDFBuildOptions* options) {
  adasdf::SDFSamplingAcceleration acceleration;
  if (!adasdf_tools::parseAccelerationAlias(value, &acceleration)) {
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
    adasdf::DenseSDFBuildOptions options;
    adasdf_tools::BuildCliRuntimeOptions runtime_options;
    std::string ignored_sampling_mode;
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
                  "adasdf_build_dense_sdf",
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
            std::cerr << "adasdf_build_dense_sdf: failed to write strict JSON: "
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
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--recommend") {
        std::cout
            << "Use adasdf_recommend_build for parameter recommendation.\n";
        return 0;
      } else if (arg == "--resolution" && hasValue(i, argc)) {
        options.resolution = std::stoi(argv[++i]);
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
      } else if ((arg == "--accel" || arg == "--distance-backend") &&
                 hasValue(i, argc)) {
        if (!parseAcceleration(argv[++i], &options)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = adasdf_tools::parseThreadsAuto(argv[++i]);
      } else if (arg == "--benchmark-brute-reference") {
        options.benchmark_brute_reference = true;
      } else if (arg == "--sampling" && hasValue(i, argc)) {
        ignored_sampling_mode = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
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
    profiler.setToolName("adasdf_build_dense_sdf");
    profiler.setInputPath(input.string());
    profiler.setOutputPath(output.string());
    adasdf::ProgressReporter progress(
        "adasdf_build_dense_sdf",
        runtime_options.progress_stderr,
        runtime_options.progress_json_path);
    adasdf::TimeoutGuard timeout(runtime_options.max_seconds);
    auto write_profile = [&]() {
      adasdf_tools::writeBuildProfile(
          "adasdf_build_dense_sdf",
          runtime_options,
          profiler);
    };
    auto timeout_exit = [&]() {
      write_strict(false, "timeout", "maximum wall-clock time reached");
      return adasdf_tools::timeoutExit(
          "adasdf_build_dense_sdf",
          runtime_options,
          &profiler,
          progress);
    };
    progress.emit("load_mesh", "validating input mesh path", 0, 1);
    if (timeout.enabled() && runtime_options.max_seconds <= 0.01) {
      return timeout_exit();
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_dense_sdf: input STL does not exist: "
                << input.string() << "\n";
      profiler.finishFailed(
          adasdf::ErrorCode::IO_ERROR,
          "input STL does not exist");
      write_profile();
      write_strict(false, "failed", "input STL does not exist");
      return 1;
    }
    if (!ignored_sampling_mode.empty() && ignored_sampling_mode != "exact") {
      std::cerr
          << "adasdf_build_dense_sdf: --sampling " << ignored_sampling_mode
          << " is ignored; hierarchical sampling applies to adaptive block "
             "builders only.\n";
    }

    adasdf::DenseSDFBuildReport report;
    std::shared_ptr<adasdf::SDFModel> model;
    {
      adasdf::BuildProfileScope scope(
          &profiler.timings.distance_query_time_ms);
      progress.emit("distance_query", "building dense SDF grid", 0, 1);
      model = adasdf::DenseSDFBuilder::fromSTL(
          input.string(),
          options,
          &report);
    }
    adasdf_tools::fillDenseProfile(&profiler, report);
    writeReports(report, report_path, json_path);
    if (!model) {
      std::cerr << "adasdf_build_dense_sdf: build failed: "
                << report.error_message << "\n";
      profiler.finishFailed(
          adasdf::ErrorCode::INVALID_MODEL,
          report.error_message);
      write_profile();
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
    if (timeout.expired()) {
      return timeout_exit();
    }

    try {
      adasdf::BuildProfileScope scope(&profiler.timings.write_model_time_ms);
      progress.emit("write_model", "writing dense SDF model", 0, 1);
      std::filesystem::path temp_output = output;
      temp_output += ".tmp";
      adasdf::SDFBinWriter::write(temp_output.string(), *model);
      auto reloaded = adasdf::SDFBinReader::read(temp_output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded DenseSDF model is invalid");
      }
      if (std::filesystem::exists(output)) {
        std::filesystem::remove(output);
      }
      std::filesystem::rename(temp_output, output);
      profiler.counters.output_bytes = std::filesystem::file_size(output);
    } catch (const std::exception& exc) {
      std::cerr << "adasdf_build_dense_sdf: write/reload validation failed: "
                << exc.what() << "\n";
      profiler.finishFailed(adasdf::ErrorCode::IO_ERROR, exc.what());
      write_profile();
      write_strict(false, "failed", exc.what());
      return 4;
    }
    progress.emit("complete", "dense SDF build complete", 1, 1);
    profiler.finishCompleted();

    std::cout << "AdaSDF-CL dense SDF builder\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Resolution: " << report.nx << " x " << report.ny << " x "
              << report.nz << "\n";
    std::cout << "Signed: " << (report.signed_distance ? "yes" : "no") << "\n";
    std::cout << "Watertight: " << (report.watertight ? "yes" : "no") << "\n";
    std::cout << "Triangles: " << report.triangle_count << "\n";
    std::cout << "Acceleration: "
              << adasdf::toString(report.acceleration_stats.acceleration)
              << "\n";
    std::cout << "Used BVH: " << (report.used_bvh ? "yes" : "no") << "\n";
    std::cout << "Threads requested: "
              << report.acceleration_stats.threads_requested << "\n";
    std::cout << "Threads used: " << report.threads_used << "\n";
    std::cout << "BVH build time ms: "
              << report.acceleration_stats.bvh_build_time_ms << "\n";
    std::cout << "Sampling time ms: "
              << report.acceleration_stats.sampling_time_ms << "\n";
    std::cout << "BVH nodes: "
              << report.acceleration_stats.bvh_node_count << "\n";
    std::cout << "BVH leaves: "
              << report.acceleration_stats.bvh_leaf_count << "\n";
    std::cout << "Brute reference time ms: "
              << report.acceleration_stats.brute_reference_time_ms << "\n";
    std::cout << "Speedup vs brute reference: "
              << report.acceleration_stats.speedup_vs_bruteforce << "\n";
    std::cout << "Build time ms: " << report.build_time_ms << "\n";
    std::cout << "Memory bytes: " << report.memory_bytes << "\n";
    std::cout << "Reload validation: success\n";
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
        {{"triangle_count", static_cast<double>(report.triangle_count)},
         {"build_time_ms", report.build_time_ms},
         {"memory_bytes", static_cast<double>(report.memory_bytes)}});
    write_profile();
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_dense_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
