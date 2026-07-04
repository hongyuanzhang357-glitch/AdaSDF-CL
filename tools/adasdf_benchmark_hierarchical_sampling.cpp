#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_hierarchical_sampling input.stl "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--target-error 1e-3] [--target-sampling-error 1e-3] "
         "[--coarse-resolution N] [--quality-check-samples N] "
         "[--comparison-samples N] [--accel brute|bvh] [--threads N] "
         "[--unsigned] [--far-field-interpolation] "
         "[--no-far-field-interpolation] [--transition-prediction] "
         "[--no-transition-prediction] [--near-surface-exact] "
         "[--quality-guard] [--no-quality-guard] "
         "[--report report.md] [--json report.json] [--csv report.csv] "
         "[--strict-json report.json] [--case-id case_id]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
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

void setTargetSamplingError(
    adasdf::AdaptiveBlockSDFBuildOptions* options,
    double value) {
  options->hierarchical_sampling.target_max_abs_error = value;
  options->hierarchical_sampling.target_rms_error = value;
  options->hierarchical_sampling.target_p95_error = value;
}

void writeReports(
    const adasdf::SpeedQualityBenchmarkResult& result,
    const std::filesystem::path& report,
    const std::filesystem::path& json,
    const std::filesystem::path& csv) {
  if (!report.empty()) {
    adasdf::HierarchicalSamplingReportWriter::writeMarkdown(
        report.string(),
        result);
  }
  if (!json.empty()) {
    adasdf::HierarchicalSamplingReportWriter::writeJson(json.string(), result);
  }
  if (!csv.empty()) {
    adasdf::HierarchicalSamplingReportWriter::writeCsv(csv.string(), result);
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
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    std::filesystem::path csv_path;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
    adasdf::SpeedQualityBenchmarkOptions options;
    options.build_options.max_octree_level = 2;
    options.build_options.block_resolution = 5;
    options.build_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.build_options.hierarchical_sampling.enable_hierarchical_sampling =
        true;
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
                  "adasdf_benchmark_hierarchical_sampling",
                  case_id,
                  input,
                  strict_json_path,
                  strict_parameters,
                  metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr
                << "adasdf_benchmark_hierarchical_sampling: failed to write "
                   "strict JSON: "
                << strict_error << "\n";
          }
        };

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.build_options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.build_options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.build_options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.build_options.target_near_surface_error = std::stod(argv[++i]);
      } else if (arg == "--target-sampling-error" && hasValue(i, argc)) {
        setTargetSamplingError(&options.build_options, std::stod(argv[++i]));
      } else if (arg == "--coarse-resolution" && hasValue(i, argc)) {
        options.build_options.hierarchical_sampling.coarse_resolution =
            std::stoi(argv[++i]);
      } else if (arg == "--quality-check-samples" && hasValue(i, argc)) {
        options.build_options.hierarchical_sampling
            .quality_check_samples_per_axis = std::stoi(argv[++i]);
      } else if (arg == "--comparison-samples" && hasValue(i, argc)) {
        options.comparison_samples_per_axis = std::stoi(argv[++i]);
      } else if (arg == "--accel" && hasValue(i, argc)) {
        if (!parseAcceleration(argv[++i], &options.build_options)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.build_options.threads = std::stoi(argv[++i]);
      } else if (arg == "--unsigned") {
        options.build_options.signed_distance = false;
        options.build_options.require_watertight_for_signed = false;
      } else if (arg == "--far-field-interpolation") {
        options.build_options.hierarchical_sampling
            .allow_far_field_interpolation = true;
      } else if (arg == "--no-far-field-interpolation") {
        options.build_options.hierarchical_sampling
            .allow_far_field_interpolation = false;
      } else if (arg == "--transition-prediction") {
        options.build_options.hierarchical_sampling
            .allow_transition_prediction = true;
      } else if (arg == "--no-transition-prediction") {
        options.build_options.hierarchical_sampling
            .allow_transition_prediction = false;
      } else if (arg == "--near-surface-exact") {
        options.build_options.hierarchical_sampling.keep_near_surface_exact =
            true;
      } else if (arg == "--quality-guard") {
        options.build_options.hierarchical_sampling.quality_guard = true;
      } else if (arg == "--no-quality-guard") {
        options.build_options.hierarchical_sampling.quality_guard = false;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty()) {
      usage();
      write_strict(false, "failed", "missing input path");
      return 1;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr
          << "adasdf_benchmark_hierarchical_sampling: input STL does not "
             "exist: "
          << input.string() << "\n";
      write_strict(false, "failed", "input STL does not exist");
      return 1;
    }

    adasdf::SpeedQualityBenchmarkResult result =
        adasdf::SpeedQualityBenchmark::runSTL(input.string(), options);
    writeReports(result, report_path, json_path, csv_path);
    if (!result.success) {
      std::cerr << "adasdf_benchmark_hierarchical_sampling: benchmark failed: "
                << result.error_message << "\n";
      write_strict(false, "failed", result.error_message);
      return 3;
    }

    std::cout << "AdaSDF-CL hierarchical sampling benchmark\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Acceleration: "
              << adasdf::toString(options.build_options.acceleration) << "\n";
    std::cout << "Exact build time ms: " << result.exact_build_time_ms << "\n";
    std::cout << "Hierarchical build time ms: "
              << result.hierarchical_build_time_ms << "\n";
    std::cout << "Speedup: " << result.speedup << "\n";
    std::cout << "Max abs error: " << result.max_abs_error << "\n";
    std::cout << "Mean abs error: " << result.mean_abs_error << "\n";
    std::cout << "RMS error: " << result.rms_error << "\n";
    std::cout << "P95 error: " << result.p95_error << "\n";
    std::cout << "Sign mismatches: " << result.sign_mismatch_count << "\n";
    std::cout << "Near-surface sign mismatches: "
              << result.near_surface_sign_mismatch_count << "\n";
    std::cout << "Exact sample count: " << result.exact_sample_count << "\n";
    std::cout << "Hierarchical exact sample count: "
              << result.hierarchical_exact_sample_count << "\n";
    std::cout << "Predicted sample count: " << result.predicted_sample_count
              << "\n";
    std::cout << "Fallback block count: " << result.fallback_block_count
              << "\n";
    std::cout << "Quality gate passed: "
              << (result.quality_gate_passed ? "yes" : "no") << "\n";
    std::cout
        << adasdf::HierarchicalSamplingReportWriter::csvHeader() << "\n"
        << adasdf::HierarchicalSamplingReportWriter::csvRow(result) << "\n";

    write_strict(
        true,
        "ok",
        "",
        {{"exact_build_time_ms", result.exact_build_time_ms},
         {"hierarchical_build_time_ms", result.hierarchical_build_time_ms},
         {"speedup", result.speedup},
         {"max_abs_error", result.max_abs_error},
         {"mean_abs_error", result.mean_abs_error},
         {"rms_error", result.rms_error},
         {"p95_error", result.p95_error},
         {"sign_mismatch_count",
          static_cast<double>(result.sign_mismatch_count)},
         {"near_surface_sign_mismatch_count",
          static_cast<double>(result.near_surface_sign_mismatch_count)},
         {"exact_sample_count", static_cast<double>(result.exact_sample_count)},
         {"hierarchical_exact_sample_count",
          static_cast<double>(result.hierarchical_exact_sample_count)},
         {"predicted_sample_count",
          static_cast<double>(result.predicted_sample_count)},
         {"fallback_block_count",
          static_cast<double>(result.fallback_block_count)}});
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_hierarchical_sampling failed: "
              << exc.what() << "\n";
    return 3;
  }
}
