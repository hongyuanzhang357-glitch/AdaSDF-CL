#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_sparse_query model.sdfbin samples.csv [--threshold value] "
         "[--phi-only] [--with-normal] [--early-exit] [--no-radius] "
         "[--include-non-colliding] [--colliding-only] [--sort] "
         "[--out results.csv] [--report sparse_report.md] "
         "[--json sparse_report.json] [--strict-json report.json] "
         "[--case-id case_id]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 3) {
      usage();
      return 1;
    }

    const std::filesystem::path model_path = argv[1];
    const std::filesystem::path samples_path = argv[2];
    adasdf::SparseSDFQueryOptions options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
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
                  "adasdf_sparse_query",
                  case_id,
                  samples_path,
                  out_path,
                  strict_parameters,
                  metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr << "adasdf_sparse_query: failed to write strict JSON: "
                      << strict_error << "\n";
          }
        };

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--phi-only") {
        options.compute_normals = false;
        options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
      } else if (arg == "--with-normal") {
        options.compute_normals = true;
        options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
      } else if (arg == "--early-exit") {
        options.early_exit = true;
      } else if (arg == "--no-radius") {
        options.use_sample_radius = false;
      } else if (arg == "--include-non-colliding") {
        options.include_non_colliding_samples = true;
      } else if (arg == "--colliding-only") {
        options.include_non_colliding_samples = false;
      } else if (arg == "--sort") {
        options.sort_results_by_effective_phi = true;
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_sparse_query: failed to load queryable model: "
                << model_path.string() << "\n";
      write_strict(false, "failed", "failed to load queryable model");
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_sparse_query: failed to read samples: "
                << samples.error_message << "\n";
      write_strict(false, "failed", samples.error_message);
      return 1;
    }

    const adasdf::SparseSDFQueryResult result =
        adasdf::SparseSDFQuery::query(*model, samples.sample_set, options);
    if (!result.success) {
      std::cerr << "adasdf_sparse_query: query failed: "
                << result.error_message << "\n";
      write_strict(false, "failed", result.error_message);
      return 2;
    }

    std::cout << "AdaSDF-CL sparse SDF query\n";
    std::cout << "Sample count: " << result.stats.sample_count << "\n";
    std::cout << "Queried count: " << result.stats.queried_count << "\n";
    std::cout << "Result count: " << result.stats.result_count << "\n";
    std::cout << "Colliding: " << (result.colliding ? "true" : "false") << "\n";
    std::cout << "Min phi: " << result.stats.min_phi << "\n";
    std::cout << "Min effective phi: " << result.stats.min_effective_phi << "\n";
    std::cout << "Early exit: "
              << (result.stats.early_exit_triggered ? "true" : "false") << "\n";
    std::cout << "Elapsed ms: " << result.stats.elapsed_ms << "\n";
    std::cout << "Output mode: "
              << adasdf::toString(options.output_mode) << "\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::SparseQueryReportWriter::writeQueryCSV(
            out_path.string(), result, &error)) {
      std::cerr << "adasdf_sparse_query: failed to write CSV: " << error << "\n";
      return 2;
    }
    if (!report_path.empty() &&
        !writeText(report_path, adasdf::SparseQueryReportWriter::queryToMarkdown(result))) {
      std::cerr << "adasdf_sparse_query: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() &&
        !writeText(json_path, adasdf::SparseQueryReportWriter::queryToJson(result))) {
      std::cerr << "adasdf_sparse_query: failed to write JSON report\n";
      return 2;
    }
    write_strict(
        true,
        "ok",
        "",
        {{"query_time_ms", result.stats.elapsed_ms},
         {"contact_count", static_cast<double>(result.stats.result_count)}});
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_sparse_query failed: " << exc.what() << "\n";
    return 2;
  }
}
