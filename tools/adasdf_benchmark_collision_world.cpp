#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_collision_world scene.csv "
         "[--mode broadphase|sparse|contacts] [--threshold value] "
         "[--repeat N] [--warmup N] [--csv benchmark.csv] "
         "[--report benchmark.md] [--json benchmark.json] "
         "[--strict-json report.json] [--case-id case_id]\n";
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
  std::size_t broadphase_pairs = 0;
  std::size_t queried_pairs = 0;
  std::size_t violations = 0;
  std::size_t solver_contacts = 0;
  double avg_total_ms = 0.0;
};

std::string csvText(
    const Metrics& metrics,
    const std::string& mode,
    int repeat,
    int warmup) {
  std::ostringstream out;
  out << "mode,broadphase_pairs,queried_pairs,violations,solver_contacts,"
         "avg_total_ms,repeat,warmup\n";
  out << mode << ","
      << metrics.broadphase_pairs << ","
      << metrics.queried_pairs << ","
      << metrics.violations << ","
      << metrics.solver_contacts << ","
      << metrics.avg_total_ms << ","
      << repeat << ","
      << warmup << "\n";
  return out.str();
}

std::string markdownText(
    const Metrics& metrics,
    const std::string& mode,
    int repeat,
    int warmup) {
  std::ostringstream out;
  out << "# CollisionWorld Benchmark\n\n";
  out << "- mode: " << mode << "\n";
  out << "- broadphase_pairs: " << metrics.broadphase_pairs << "\n";
  out << "- queried_pairs: " << metrics.queried_pairs << "\n";
  out << "- violations: " << metrics.violations << "\n";
  out << "- solver_contacts: " << metrics.solver_contacts << "\n";
  out << "- avg_total_ms: " << metrics.avg_total_ms << "\n";
  out << "- repeat: " << repeat << "\n";
  out << "- warmup: " << warmup << "\n\n";
  out << "This benchmark measures world orchestration, not exact mesh-vs-mesh contact or a solver.\n";
  return out.str();
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    const std::filesystem::path scene_path = argv[1];
    std::string mode = "sparse";
    int repeat = 10;
    int warmup = 1;
    double threshold = 0.0;
    std::filesystem::path csv_path;
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
            const std::map<std::string, double>& strict_metrics = {}) {
          if (strict_json_path.empty()) {
            return;
          }
          std::string strict_error;
          if (!adasdf::writeStrictRunReport(
                  strict_json_path,
                  "adasdf_benchmark_collision_world",
                  case_id,
                  scene_path,
                  csv_path,
                  strict_parameters,
                  strict_metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr << "adasdf_benchmark_collision_world: failed to write "
                         "strict JSON: "
                      << strict_error << "\n";
          }
        };

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--mode" && hasValue(i, argc)) {
        mode = argv[++i];
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        threshold = std::stod(argv[++i]);
      } else if (arg == "--repeat" && hasValue(i, argc)) {
        repeat = std::max(1, std::stoi(argv[++i]));
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        warmup = std::max(0, std::stoi(argv[++i]));
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
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

    const adasdf::WorldSceneReadResult scene =
        adasdf::WorldSceneIO::readCSV(scene_path);
    if (!scene.success) {
      std::cerr << "adasdf_benchmark_collision_world: failed to read scene: "
                << scene.error_message << "\n";
      write_strict(false, "failed", scene.error_message);
      return 1;
    }

    Metrics metrics;
    double total_ms = 0.0;
    const int iterations = warmup + repeat;
    for (int i = 0; i < iterations; ++i) {
      const auto start = std::chrono::steady_clock::now();
      if (mode == "broadphase") {
        const adasdf::AABBBroadphaseResult result =
            adasdf::AABBBroadphase::compute(scene.world);
        if (!result.success) {
          write_strict(false, "failed", result.error_message);
          return 2;
        }
        metrics.broadphase_pairs = result.stats.overlap_pair_count;
      } else if (mode == "sparse") {
        adasdf::WorldSparseCollisionOptions options;
        options.mode = adasdf::SparseCollisionMode::CandidateSearch;
        options.threshold = threshold;
        options.early_exit = false;
        const adasdf::WorldSparseCollisionResult result =
            adasdf::WorldSparseCollision::check(scene.world, options);
        if (!result.success) {
          write_strict(false, "failed", result.error_message);
          return 2;
        }
        metrics.broadphase_pairs = result.stats.broadphase_pair_count;
        metrics.queried_pairs = result.stats.queried_pair_count;
        metrics.violations = result.stats.violation_count;
      } else if (mode == "contacts") {
        adasdf::WorldSolverContactOptions options;
        options.collision_options.threshold = threshold;
        const adasdf::WorldSolverContactResult result =
            adasdf::WorldSolverContacts::build(scene.world, options);
        if (!result.success) {
          write_strict(false, "failed", result.error_message);
          return 2;
        }
        metrics.broadphase_pairs =
            result.sparse_result.stats.broadphase_pair_count;
        metrics.queried_pairs = result.sparse_result.stats.queried_pair_count;
        metrics.violations = result.sparse_result.stats.violation_count;
        metrics.solver_contacts = result.contacts.size();
      } else {
        std::cerr << "mode must be broadphase, sparse, or contacts\n";
        write_strict(false, "failed", "mode must be broadphase, sparse, or contacts");
        return 1;
      }
      const auto end = std::chrono::steady_clock::now();
      if (i >= warmup) {
        total_ms +=
            std::chrono::duration<double, std::milli>(end - start).count();
      }
    }
    metrics.avg_total_ms = total_ms / static_cast<double>(repeat);

    std::cout << "AdaSDF-CL CollisionWorld benchmark\n";
    std::cout << "mode: " << mode << "\n";
    std::cout << "broadphase_pairs: " << metrics.broadphase_pairs << "\n";
    std::cout << "queried_pairs: " << metrics.queried_pairs << "\n";
    std::cout << "violations: " << metrics.violations << "\n";
    std::cout << "solver_contacts: " << metrics.solver_contacts << "\n";
    std::cout << "avg_total_ms: " << metrics.avg_total_ms << "\n";
    std::cout << "repeat: " << repeat << "\n";
    std::cout << "warmup: " << warmup << "\n";
    std::cout << "Status: ok\n";

    if (!csv_path.empty() &&
        !writeText(csv_path, csvText(metrics, mode, repeat, warmup))) {
      std::cerr << "adasdf_benchmark_collision_world: failed to write CSV\n";
      return 3;
    }
    if (!report_path.empty() &&
        !writeText(report_path, markdownText(metrics, mode, repeat, warmup))) {
      std::cerr << "adasdf_benchmark_collision_world: failed to write report\n";
      return 3;
    }
    if (!json_path.empty() &&
        !writeText(
            json_path,
            std::string("{\n") +
                "  \"mode\": \"" + mode + "\",\n" +
                "  \"broadphase_pairs\": " +
                std::to_string(metrics.broadphase_pairs) + ",\n" +
                "  \"queried_pairs\": " +
                std::to_string(metrics.queried_pairs) + ",\n" +
                "  \"violations\": " +
                std::to_string(metrics.violations) + ",\n" +
                "  \"solver_contacts\": " +
                std::to_string(metrics.solver_contacts) + ",\n" +
                "  \"avg_total_ms\": " +
                std::to_string(metrics.avg_total_ms) + "\n" +
                "}\n")) {
      std::cerr << "adasdf_benchmark_collision_world: failed to write JSON\n";
      return 3;
    }
    write_strict(
        true,
        "ok",
        "",
        {{"benchmark_ns_per_query", metrics.avg_total_ms * 1000000.0},
         {"contact_count", static_cast<double>(metrics.violations)},
         {"solver_contact_count", static_cast<double>(metrics.solver_contacts)},
         {"broadphase_pair_count",
          static_cast<double>(metrics.broadphase_pairs)}});
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_collision_world failed: "
              << exc.what() << "\n";
    return 2;
  }
}
