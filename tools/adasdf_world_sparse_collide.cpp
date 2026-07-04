#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_world_sparse_collide scene.csv "
         "[--mode collision-only|clearance|candidate-search] "
         "[--threshold value] [--one-way] [--bidirectional] "
         "[--early-exit] [--no-early-exit] [--with-normal] [--no-radius] "
         "[--include-static-static] [--no-group-mask] [--out hits.csv] "
         "[--report report.md] [--json report.json]\n"
      << "Return code 10 means success with collision detected.\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    const std::filesystem::path scene_path = argv[1];
    adasdf::WorldSparseCollisionOptions options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    bool early_exit_explicit = false;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--mode" && hasValue(i, argc)) {
        options.mode = adasdf::sparseCollisionModeFromString(argv[++i]);
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--one-way") {
        options.bidirectional = false;
      } else if (arg == "--bidirectional") {
        options.bidirectional = true;
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
      } else if (arg == "--include-static-static") {
        options.broadphase_options.include_static_static = true;
      } else if (arg == "--no-group-mask") {
        options.broadphase_options.use_group_mask = false;
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
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

    const adasdf::WorldSceneReadResult scene =
        adasdf::WorldSceneIO::readCSV(scene_path);
    if (!scene.success) {
      std::cerr << "adasdf_world_sparse_collide: failed to read scene: "
                << scene.error_message << "\n";
      return 1;
    }

    const adasdf::WorldSparseCollisionResult result =
        adasdf::WorldSparseCollision::check(scene.world, options);
    if (!result.success) {
      std::cerr << "adasdf_world_sparse_collide: query failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL CollisionWorld sparse collision\n";
    std::cout << "Mode: " << adasdf::toString(options.mode) << "\n";
    std::cout << "Colliding: " << (result.colliding ? "true" : "false") << "\n";
    std::cout << "Broadphase pairs: "
              << result.stats.broadphase_pair_count << "\n";
    std::cout << "Queried pairs: " << result.stats.queried_pair_count << "\n";
    std::cout << "Queried samples: "
              << result.stats.queried_sample_count << "\n";
    std::cout << "Violations: " << result.stats.violation_count << "\n";
    std::cout << "Min effective phi: " << result.stats.min_effective_phi << "\n";
    std::cout << "Sample-based SDF collision, not exact mesh-vs-mesh contact.\n";
    std::cout << "Return code note: 10 means collision detected, not failure\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::WorldReportWriter::writeSparseCSV(out_path, result, &error)) {
      std::cerr << "adasdf_world_sparse_collide: failed to write CSV: "
                << error << "\n";
      return 3;
    }
    if (!report_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            report_path,
            adasdf::WorldReportWriter::sparseToMarkdown(result),
            &error)) {
      std::cerr << "adasdf_world_sparse_collide: failed to write report: "
                << error << "\n";
      return 3;
    }
    if (!json_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            json_path,
            adasdf::WorldReportWriter::sparseToJson(result),
            &error)) {
      std::cerr << "adasdf_world_sparse_collide: failed to write JSON: "
                << error << "\n";
      return 3;
    }
    return (options.mode == adasdf::SparseCollisionMode::CollisionOnly &&
            result.colliding)
               ? 10
               : 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_world_sparse_collide failed: " << exc.what() << "\n";
    return 2;
  }
}
