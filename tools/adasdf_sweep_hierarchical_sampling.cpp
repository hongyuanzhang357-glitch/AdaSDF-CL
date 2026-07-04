#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_sweep_hierarchical_sampling input.stl "
         "[--max-level N[,N...]] [--block-resolution N] "
         "[--target-error value] [--target-sampling-error value] "
         "[--coarse-resolution N[,N...]] "
         "[--transition-quality-check-samples N[,N...]] "
         "[--far-field-quality-check corners[,sparse,full,none]] "
         "[--far-field-safety-factor value[,value...]] "
         "[--quality-check-samples N] [--comparison-samples N] "
         "[--accel brute|bvh] [--threads N] [--unsigned] "
         "[--csv sweep.csv] [--report sweep.md]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::stringstream in(text);
  std::string item;
  while (std::getline(in, item, ',')) {
    if (!item.empty()) {
      values.push_back(item);
    }
  }
  return values;
}

std::vector<int> parseIntList(const std::string& text) {
  std::vector<int> values;
  for (const std::string& item : splitList(text)) {
    values.push_back(std::stoi(item));
  }
  return values;
}

std::vector<double> parseDoubleList(const std::string& text) {
  std::vector<double> values;
  for (const std::string& item : splitList(text)) {
    values.push_back(std::stod(item));
  }
  return values;
}

std::vector<adasdf::FarFieldQualityCheckMode> parseModeList(
    const std::string& text) {
  std::vector<adasdf::FarFieldQualityCheckMode> values;
  for (const std::string& item : splitList(text)) {
    adasdf::FarFieldQualityCheckMode mode;
    if (!adasdf::parseFarFieldQualityCheckMode(item, &mode)) {
      throw std::runtime_error("unknown far-field quality check mode: " + item);
    }
    values.push_back(mode);
  }
  return values;
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

void setTargetSamplingError(
    adasdf::AdaptiveBlockSDFBuildOptions* options,
    double value) {
  options->hierarchical_sampling.target_max_abs_error = value;
  options->hierarchical_sampling.target_rms_error = value;
  options->hierarchical_sampling.target_p95_error = value;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path csv_path;
    std::filesystem::path report_path;

    adasdf::SpeedQualityBenchmarkOptions base;
    base.build_options.max_octree_level = 4;
    base.build_options.block_resolution = 8;
    base.build_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    base.build_options.hierarchical_sampling.enable_hierarchical_sampling = true;
    base.build_options.hierarchical_sampling.hierarchical_diagnostics = true;
    std::vector<int> max_levels{4};
    std::vector<int> coarse_resolutions{3};
    std::vector<int> transition_samples{2};
    std::vector<adasdf::FarFieldQualityCheckMode> far_modes{
        adasdf::FarFieldQualityCheckMode::Corners};
    std::vector<double> far_safety_factors{2.0};

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        max_levels = parseIntList(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        base.build_options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        base.build_options.target_near_surface_error = std::stod(argv[++i]);
      } else if (arg == "--target-sampling-error" && hasValue(i, argc)) {
        setTargetSamplingError(&base.build_options, std::stod(argv[++i]));
      } else if (arg == "--coarse-resolution" && hasValue(i, argc)) {
        coarse_resolutions = parseIntList(argv[++i]);
      } else if (arg == "--transition-quality-check-samples" &&
                 hasValue(i, argc)) {
        transition_samples = parseIntList(argv[++i]);
      } else if (arg == "--far-field-quality-check" && hasValue(i, argc)) {
        far_modes = parseModeList(argv[++i]);
      } else if (arg == "--far-field-safety-factor" && hasValue(i, argc)) {
        far_safety_factors = parseDoubleList(argv[++i]);
      } else if (arg == "--quality-check-samples" && hasValue(i, argc)) {
        base.build_options.hierarchical_sampling
            .quality_check_samples_per_axis = std::stoi(argv[++i]);
      } else if (arg == "--comparison-samples" && hasValue(i, argc)) {
        base.comparison_samples_per_axis = std::stoi(argv[++i]);
      } else if (arg == "--threads" && hasValue(i, argc)) {
        base.build_options.threads = std::stoi(argv[++i]);
      } else if (arg == "--accel" && hasValue(i, argc)) {
        adasdf::SDFSamplingAcceleration acceleration;
        if (!adasdf::parseSDFSamplingAcceleration(argv[++i], &acceleration)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
        base.build_options.acceleration = acceleration;
      } else if (arg == "--unsigned") {
        base.build_options.signed_distance = false;
        base.build_options.require_watertight_for_signed = false;
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
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

    if (input.empty() || !std::filesystem::exists(input)) {
      std::cerr << "adasdf_sweep_hierarchical_sampling: input STL does not "
                   "exist\n";
      return 1;
    }

    std::vector<adasdf::SpeedQualityBenchmarkResult> results;
    std::size_t case_index = 0;
    for (const int max_level : max_levels) {
      for (const int coarse_resolution : coarse_resolutions) {
        for (const int transition_sample_count : transition_samples) {
          for (const auto far_mode : far_modes) {
            for (const double safety_factor : far_safety_factors) {
              adasdf::SpeedQualityBenchmarkOptions options = base;
              options.build_options.max_octree_level = max_level;
              options.build_options.hierarchical_sampling.coarse_resolution =
                  coarse_resolution;
              options.build_options.hierarchical_sampling
                  .transition_quality_check_samples_per_axis =
                  transition_sample_count;
              options.build_options.hierarchical_sampling
                  .far_field_quality_check = far_mode;
              options.build_options.hierarchical_sampling
                  .far_field_safety_factor = safety_factor;

              adasdf::SpeedQualityBenchmarkResult result =
                  adasdf::SpeedQualityBenchmark::runSTL(
                      input.string(),
                      options);
              result.case_id = "sweep_" + std::to_string(case_index++);
              results.push_back(result);
            }
          }
        }
      }
    }

    const adasdf::SpeedQualityBenchmarkResult* best = nullptr;
    for (const auto& result : results) {
      if (!result.effective_speedup_claim_allowed) {
        continue;
      }
      if (best == nullptr || result.speedup > best->speedup) {
        best = &result;
      }
    }

    if (!csv_path.empty()) {
      ensureParent(csv_path);
      std::ofstream csv(csv_path);
      csv << adasdf::HierarchicalSamplingReportWriter::csvHeader() << "\n";
      for (const auto& result : results) {
        csv << adasdf::HierarchicalSamplingReportWriter::csvRow(result)
            << "\n";
      }
    }

    if (!report_path.empty()) {
      ensureParent(report_path);
      std::ofstream report(report_path);
      report << "# Hierarchical Sampling Sweep\n\n";
      report << "- Input: " << input.string() << "\n";
      report << "- Cases: " << results.size() << "\n";
      report << "- Best quality-pass speedup case: "
             << (best == nullptr ? "none" : best->case_id) << "\n\n";
      report << "| case | max_level | coarse | transition_check | far_check | "
                "safety | speedup | quality | effective |\n";
      report << "| --- | ---: | ---: | ---: | --- | ---: | ---: | --- | --- |\n";
      for (const auto& result : results) {
        report << "| " << result.case_id << " | " << result.max_level << " | "
               << result.coarse_resolution << " | "
               << result.transition_quality_check_samples << " | "
               << adasdf::toString(result.far_field_quality_check) << " | "
               << result.far_field_safety_factor << " | " << result.speedup
               << " | " << (result.quality_gate_passed ? "PASS" : "FAIL")
               << " | "
               << (result.effective_speedup_claim_allowed ? "yes" : "no")
               << " |\n";
      }
    }

    std::cout << "AdaSDF-CL hierarchical sampling sweep\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Cases: " << results.size() << "\n";
    if (best != nullptr) {
      std::cout << "Best quality-pass speedup: " << best->case_id << " "
                << best->speedup << "\n";
    } else {
      std::cout << "Best quality-pass speedup: none\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_sweep_hierarchical_sampling failed: " << exc.what()
              << "\n";
    return 3;
  }
}
