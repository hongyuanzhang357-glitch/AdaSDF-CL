#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_sparse_collide model.sdfbin samples.csv "
         "[--mode collision-only|clearance|candidate-search] [--threshold value] "
         "[--early-exit] [--no-early-exit] [--with-normal] [--no-radius] "
         "[--return-all-violations] [--report collision_report.md] "
         "[--json collision_report.json]\n"
      << "Return code 10 means success with collision detected.\n";
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
    adasdf::SparseCollisionQueryOptions options;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    bool early_exit_explicit = false;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--mode" && hasValue(i, argc)) {
        options.mode = adasdf::sparseCollisionModeFromString(argv[++i]);
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--early-exit") {
        options.early_exit = true;
        early_exit_explicit = true;
      } else if (arg == "--no-early-exit") {
        options.early_exit = false;
        early_exit_explicit = true;
      } else if (arg == "--with-normal") {
        options.compute_normals = true;
      } else if (arg == "--no-radius") {
        options.use_sample_radius = false;
      } else if (arg == "--return-all-violations") {
        options.return_all_violations = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (!early_exit_explicit &&
        (options.mode == adasdf::SparseCollisionMode::Clearance ||
         options.mode == adasdf::SparseCollisionMode::CandidateSearch)) {
      options.early_exit = false;
    }

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_sparse_collide: failed to load queryable model\n";
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_sparse_collide: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const adasdf::SparseCollisionResult result =
        adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
    if (!result.success) {
      std::cerr << "adasdf_sparse_collide: query failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL sparse collision query\n";
    std::cout << "Mode: " << adasdf::toString(options.mode) << "\n";
    std::cout << "Colliding: " << (result.colliding ? "true" : "false") << "\n";
    std::cout << "Min phi: " << result.min_phi << "\n";
    std::cout << "Min effective phi: " << result.min_effective_phi << "\n";
    std::cout << "First hit sample id: " << result.first_hit_sample_id << "\n";
    std::cout << "Sample count: " << result.sample_count << "\n";
    std::cout << "Queried samples: " << result.queried_count << "\n";
    std::cout << "Early exit: "
              << (result.early_exit_triggered ? "true" : "false") << "\n";
    std::cout << "Elapsed ms: " << result.elapsed_ms << "\n";
    std::cout << "Return code note: 10 means collision detected, not failure\n";
    std::cout << "Status: ok\n";

    if (!report_path.empty() &&
        !writeText(
            report_path,
            adasdf::SparseQueryReportWriter::collisionToMarkdown(result))) {
      std::cerr << "adasdf_sparse_collide: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() &&
        !writeText(json_path, adasdf::SparseQueryReportWriter::collisionToJson(result))) {
      std::cerr << "adasdf_sparse_collide: failed to write JSON report\n";
      return 2;
    }
    return (options.mode == adasdf::SparseCollisionMode::CollisionOnly &&
            result.colliding)
               ? 10
               : 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_sparse_collide failed: " << exc.what() << "\n";
    return 2;
  }
}
