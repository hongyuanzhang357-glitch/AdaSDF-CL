#include <adasdf/adasdf.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_world_solver_contacts scene.csv "
         "[--threshold value] [--top-k N] [--reduction-radius value] "
         "[--max-contacts N] [--patch-radius value] [--no-radius] "
         "[--include-static-static] [--out contacts.csv] [--report report.md] "
         "[--json report.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

double maxPenetration(const adasdf::SolverContactSet& contacts) {
  double value = 0.0;
  for (const adasdf::SolverContact& contact : contacts.contacts) {
    value = std::max(value, contact.penetration_depth);
  }
  return value;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    const std::filesystem::path scene_path = argv[1];
    adasdf::WorldSolverContactOptions options;
    options.collision_options.threshold = 1e-3;
    options.collision_options.mode = adasdf::SparseCollisionMode::CandidateSearch;
    options.collision_options.early_exit = false;
    options.collision_options.compute_normals = true;
    options.candidate_options.top_k = 64;
    options.candidate_options.candidate_threshold =
        options.collision_options.threshold;
    options.candidate_options.compute_normals = true;
    options.stabilization_options.budget.max_contacts_total = 8;

    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        options.collision_options.threshold = std::stod(argv[++i]);
        options.candidate_options.candidate_threshold =
            options.collision_options.threshold;
      } else if (arg == "--top-k" && hasValue(i, argc)) {
        options.candidate_options.top_k = std::stoi(argv[++i]);
      } else if (arg == "--reduction-radius" && hasValue(i, argc)) {
        options.candidate_options.reduction_radius = std::stod(argv[++i]);
      } else if (arg == "--max-contacts" && hasValue(i, argc)) {
        options.stabilization_options.budget.max_contacts_total =
            std::stoi(argv[++i]);
      } else if (arg == "--patch-radius" && hasValue(i, argc)) {
        options.stabilization_options.patch_options.spatial_radius =
            std::stod(argv[++i]);
      } else if (arg == "--no-radius") {
        options.collision_options.use_sample_radius = false;
      } else if (arg == "--include-static-static") {
        options.collision_options.broadphase_options.include_static_static = true;
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

    const adasdf::WorldSceneReadResult scene =
        adasdf::WorldSceneIO::readCSV(scene_path);
    if (!scene.success) {
      std::cerr << "adasdf_world_solver_contacts: failed to read scene: "
                << scene.error_message << "\n";
      return 1;
    }

    const adasdf::WorldSolverContactResult result =
        adasdf::WorldSolverContacts::build(scene.world, options);
    if (!result.success) {
      std::cerr << "adasdf_world_solver_contacts: query failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL CollisionWorld solver-ready contacts\n";
    std::cout << "Raw candidates: " << result.stats.raw_candidate_count << "\n";
    std::cout << "Reduced candidates: "
              << result.stats.reduced_candidate_count << "\n";
    std::cout << "Solver contacts: " << result.contacts.size() << "\n";
    std::cout << "Patches: " << result.stats.patch_count << "\n";
    std::cout << "Max penetration: " << maxPenetration(result.contacts) << "\n";
    std::cout << "This exports solver-ready candidates, not solver constraints.\n";
    std::cout << "No impulses or friction forces are computed.\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::WorldReportWriter::writeSolverContactsCSV(
            out_path, result, &error)) {
      std::cerr << "adasdf_world_solver_contacts: failed to write CSV: "
                << error << "\n";
      return 3;
    }
    if (!report_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            report_path,
            adasdf::WorldReportWriter::solverContactsToMarkdown(result),
            &error)) {
      std::cerr << "adasdf_world_solver_contacts: failed to write report: "
                << error << "\n";
      return 3;
    }
    if (!json_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            json_path,
            adasdf::WorldReportWriter::solverContactsToJson(result),
            &error)) {
      std::cerr << "adasdf_world_solver_contacts: failed to write JSON: "
                << error << "\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_world_solver_contacts failed: "
              << exc.what() << "\n";
    return 2;
  }
}
