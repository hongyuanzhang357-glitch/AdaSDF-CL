#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_world_broadphase scene.csv "
         "[--include-disabled] [--include-static-static] [--no-group-mask] "
         "[--aabb-margin value] [--out pairs.csv] [--report report.md] "
         "[--json report.json]\n";
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
    adasdf::AABBBroadphaseOptions options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--include-disabled") {
        options.include_disabled = true;
      } else if (arg == "--include-static-static") {
        options.include_static_static = true;
      } else if (arg == "--no-group-mask") {
        options.use_group_mask = false;
      } else if (arg == "--aabb-margin" && hasValue(i, argc)) {
        options.aabb_margin = std::stod(argv[++i]);
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
      std::cerr << "adasdf_world_broadphase: failed to read scene: "
                << scene.error_message << "\n";
      return 1;
    }

    const adasdf::AABBBroadphaseResult result =
        adasdf::AABBBroadphase::compute(scene.world, options);
    if (!result.success) {
      std::cerr << "adasdf_world_broadphase: query failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL CollisionWorld broadphase\n";
    std::cout << "Objects: " << result.stats.object_count << "\n";
    std::cout << "Tested pairs: " << result.stats.tested_pair_count << "\n";
    std::cout << "Overlap pairs: " << result.stats.overlap_pair_count << "\n";
    std::cout << "Static-static skipped: "
              << result.stats.static_static_pair_skipped << "\n";
    std::cout << "Group/mask skipped: "
              << result.stats.group_mask_pair_skipped << "\n";
    std::cout << "AABB rejected: " << result.stats.aabb_rejected_count << "\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::WorldReportWriter::writeBroadphaseCSV(
            out_path, result, &error)) {
      std::cerr << "adasdf_world_broadphase: failed to write CSV: "
                << error << "\n";
      return 3;
    }
    if (!report_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            report_path,
            adasdf::WorldReportWriter::broadphaseToMarkdown(result),
            &error)) {
      std::cerr << "adasdf_world_broadphase: failed to write report: "
                << error << "\n";
      return 3;
    }
    if (!json_path.empty() &&
        !adasdf::WorldReportWriter::writeText(
            json_path,
            adasdf::WorldReportWriter::broadphaseToJson(result),
            &error)) {
      std::cerr << "adasdf_world_broadphase: failed to write JSON: "
                << error << "\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_world_broadphase failed: " << exc.what() << "\n";
    return 2;
  }
}
