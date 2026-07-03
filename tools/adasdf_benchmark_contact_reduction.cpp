#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_contact_reduction model.sdfbin samples.csv "
         "[--threshold value] [--top-k N] [--max-contacts N] "
         "[--patch-radius value] [--repeat N] [--warmup N] "
         "[--report benchmark.md] [--json benchmark.json] [--csv benchmark.csv]\n";
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

struct Metrics {
  std::size_t sample_count = 0;
  std::size_t raw_candidate_count = 0;
  std::size_t patch_count = 0;
  std::size_t solver_contact_count = 0;
  double avg_query_ms = 0.0;
  double avg_reduction_ms = 0.0;
  double avg_total_ms = 0.0;
};

std::string csvText(
    const Metrics& metrics,
    int max_contacts,
    double patch_radius,
    int repeat,
    int warmup) {
  std::ostringstream out;
  out << "sample_count,raw_candidate_count,patch_count,solver_contact_count,"
         "candidate_reduction_ratio,avg_query_ms,avg_reduction_ms,avg_total_ms,"
         "max_contacts,patch_radius,repeat,warmup\n";
  const double ratio = metrics.raw_candidate_count == 0
      ? 0.0
      : static_cast<double>(metrics.solver_contact_count) /
            static_cast<double>(metrics.raw_candidate_count);
  out << metrics.sample_count << ","
      << metrics.raw_candidate_count << ","
      << metrics.patch_count << ","
      << metrics.solver_contact_count << ","
      << ratio << ","
      << metrics.avg_query_ms << ","
      << metrics.avg_reduction_ms << ","
      << metrics.avg_total_ms << ","
      << max_contacts << ","
      << patch_radius << ","
      << repeat << ","
      << warmup << "\n";
  return out.str();
}

std::string markdownText(
    const Metrics& metrics,
    int max_contacts,
    double patch_radius,
    int repeat,
    int warmup) {
  std::ostringstream out;
  out << "# Contact Reduction Benchmark\n\n";
  out << "- sample_count: " << metrics.sample_count << "\n";
  out << "- raw_candidate_count: " << metrics.raw_candidate_count << "\n";
  out << "- patch_count: " << metrics.patch_count << "\n";
  out << "- solver_contact_count: " << metrics.solver_contact_count << "\n";
  out << "- candidate_reduction_ratio: "
      << (metrics.raw_candidate_count == 0
              ? 0.0
              : static_cast<double>(metrics.solver_contact_count) /
                    static_cast<double>(metrics.raw_candidate_count))
      << "\n";
  out << "- avg_query_ms: " << metrics.avg_query_ms << "\n";
  out << "- avg_reduction_ms: " << metrics.avg_reduction_ms << "\n";
  out << "- avg_total_ms: " << metrics.avg_total_ms << "\n";
  out << "- max_contacts: " << max_contacts << "\n";
  out << "- patch_radius: " << patch_radius << "\n";
  out << "- repeat: " << repeat << "\n";
  out << "- warmup: " << warmup << "\n\n";
  out << "This benchmark measures candidate reduction and stabilization, not a contact solver.\n";
  return out.str();
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

    adasdf::SparseSDFQueryOptions query_options;
    query_options.threshold = 1e-3;
    query_options.compute_normals = true;
    query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
    query_options.include_non_colliding_samples = true;
    query_options.sort_results_by_effective_phi = true;

    adasdf::ContactCandidateOptions candidate_options;
    candidate_options.top_k = 64;
    candidate_options.candidate_threshold = query_options.threshold;
    candidate_options.compute_normals = true;

    adasdf::ContactStabilizationOptions stabilize_options;
    stabilize_options.budget.max_contacts_total = 8;

    int repeat = 10;
    int warmup = 1;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    std::filesystem::path csv_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        query_options.threshold = std::stod(argv[++i]);
        candidate_options.candidate_threshold = query_options.threshold;
      } else if (arg == "--top-k" && hasValue(i, argc)) {
        candidate_options.top_k = std::stoi(argv[++i]);
      } else if (arg == "--max-contacts" && hasValue(i, argc)) {
        stabilize_options.budget.max_contacts_total = std::stoi(argv[++i]);
      } else if (arg == "--patch-radius" && hasValue(i, argc)) {
        stabilize_options.patch_options.spatial_radius = std::stod(argv[++i]);
      } else if (arg == "--repeat" && hasValue(i, argc)) {
        repeat = std::max(1, std::stoi(argv[++i]));
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        warmup = std::max(0, std::stoi(argv[++i]));
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
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
      std::cerr << "adasdf_benchmark_contact_reduction: failed to load queryable model\n";
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_benchmark_contact_reduction: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    Metrics metrics;
    double query_total = 0.0;
    double reduction_total = 0.0;
    double total_total = 0.0;
    const int iterations = warmup + repeat;
    for (int i = 0; i < iterations; ++i) {
      const auto total_start = std::chrono::steady_clock::now();
      const adasdf::SparseSDFQueryResult query =
          adasdf::SparseSDFQuery::query(*model, samples.sample_set, query_options);
      if (!query.success) {
        std::cerr << "adasdf_benchmark_contact_reduction: sparse query failed: "
                  << query.error_message << "\n";
        return 2;
      }
      const auto reduction_start = std::chrono::steady_clock::now();
      const adasdf::ContactCandidateReductionResult reduced =
          adasdf::ContactCandidateReducer::reduce(query.samples, candidate_options);
      const adasdf::ContactStabilizationResult stabilized =
          adasdf::ContactStabilizer::stabilize(
              reduced.candidates,
              stabilize_options);
      if (!stabilized.success) {
        std::cerr << "adasdf_benchmark_contact_reduction: stabilization failed: "
                  << stabilized.error_message << "\n";
        return 2;
      }
      const adasdf::SolverContactSet contacts =
          adasdf::SolverContactBuilder::fromCandidates(
              stabilized.stabilized_candidates,
              stabilized.patches);
      const auto end = std::chrono::steady_clock::now();
      if (i >= warmup) {
        query_total += query.stats.elapsed_ms;
        reduction_total += std::chrono::duration<double, std::milli>(
                               end - reduction_start)
                               .count();
        total_total += std::chrono::duration<double, std::milli>(
                           end - total_start)
                           .count();
      }
      metrics.sample_count = query.stats.sample_count;
      metrics.raw_candidate_count = reduced.reduced_count;
      metrics.patch_count = stabilized.stats.patch_count;
      metrics.solver_contact_count = contacts.size();
    }

    metrics.avg_query_ms = query_total / static_cast<double>(repeat);
    metrics.avg_reduction_ms = reduction_total / static_cast<double>(repeat);
    metrics.avg_total_ms = total_total / static_cast<double>(repeat);
    const double ratio = metrics.raw_candidate_count == 0
        ? 0.0
        : static_cast<double>(metrics.solver_contact_count) /
              static_cast<double>(metrics.raw_candidate_count);

    std::cout << "AdaSDF-CL contact reduction benchmark\n";
    std::cout << "sample_count: " << metrics.sample_count << "\n";
    std::cout << "raw_candidate_count: " << metrics.raw_candidate_count << "\n";
    std::cout << "patch_count: " << metrics.patch_count << "\n";
    std::cout << "solver_contact_count: " << metrics.solver_contact_count << "\n";
    std::cout << "candidate_reduction_ratio: " << ratio << "\n";
    std::cout << "avg_query_ms: " << metrics.avg_query_ms << "\n";
    std::cout << "avg_reduction_ms: " << metrics.avg_reduction_ms << "\n";
    std::cout << "avg_total_ms: " << metrics.avg_total_ms << "\n";
    std::cout << "max_contacts: " << stabilize_options.budget.max_contacts_total << "\n";
    std::cout << "patch_radius: " << stabilize_options.patch_options.spatial_radius << "\n";
    std::cout << "repeat: " << repeat << "\n";
    std::cout << "warmup: " << warmup << "\n";
    std::cout << "Status: ok\n";

    if (!csv_path.empty() &&
        !writeText(
            csv_path,
            csvText(
                metrics,
                stabilize_options.budget.max_contacts_total,
                stabilize_options.patch_options.spatial_radius,
                repeat,
                warmup))) {
      std::cerr << "adasdf_benchmark_contact_reduction: failed to write CSV\n";
      return 3;
    }
    if (!report_path.empty() &&
        !writeText(
            report_path,
            markdownText(
                metrics,
                stabilize_options.budget.max_contacts_total,
                stabilize_options.patch_options.spatial_radius,
                repeat,
                warmup))) {
      std::cerr << "adasdf_benchmark_contact_reduction: failed to write report\n";
      return 3;
    }
    if (!json_path.empty() &&
        !writeText(
            json_path,
            std::string("{\n") +
                "  \"sample_count\": " + std::to_string(metrics.sample_count) + ",\n" +
                "  \"raw_candidate_count\": " + std::to_string(metrics.raw_candidate_count) + ",\n" +
                "  \"patch_count\": " + std::to_string(metrics.patch_count) + ",\n" +
                "  \"solver_contact_count\": " + std::to_string(metrics.solver_contact_count) + ",\n" +
                "  \"candidate_reduction_ratio\": " + std::to_string(ratio) + ",\n" +
                "  \"avg_query_ms\": " + std::to_string(metrics.avg_query_ms) + ",\n" +
                "  \"avg_reduction_ms\": " + std::to_string(metrics.avg_reduction_ms) + ",\n" +
                "  \"avg_total_ms\": " + std::to_string(metrics.avg_total_ms) + "\n" +
                "}\n")) {
      std::cerr << "adasdf_benchmark_contact_reduction: failed to write JSON\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_contact_reduction failed: "
              << exc.what() << "\n";
    return 2;
  }
}
