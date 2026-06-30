#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_mesh_clean input.stl output_cleaned.stl "
         "[--report cleanup_report.md] [--merge-tolerance 1e-12] "
         "[--area-eps 1e-14] [--no-merge-vertices] "
         "[--no-remove-degenerate] [--no-remove-duplicates] "
         "[--no-remove-unused]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  file << text;
}

void printCleanup(const adasdf::MeshCleanupStats& stats) {
  std::cout << "AdaSDF-CL safe mesh cleanup\n";
  std::cout << "Input vertices: " << stats.input_vertices << "\n";
  std::cout << "Input triangles: " << stats.input_triangles << "\n";
  std::cout << "Output vertices: " << stats.output_vertices << "\n";
  std::cout << "Output triangles: " << stats.output_triangles << "\n";
  std::cout << "Merged vertices: " << stats.merged_vertices << "\n";
  std::cout << "Removed degenerate triangles: "
            << stats.removed_degenerate_triangles << "\n";
  std::cout << "Removed duplicate triangles: "
            << stats.removed_duplicate_triangles << "\n";
  std::cout << "Removed unused vertices: " << stats.removed_unused_vertices
            << "\n";
  std::cout << "Topology may have changed: "
            << (stats.topology_may_have_changed ? "yes" : "no") << "\n";
  for (const std::string& warning : stats.warnings) {
    std::cout << "Warning: " << warning << "\n";
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
    std::filesystem::path output;
    std::filesystem::path report_path;
    adasdf::STLReadOptions read_options;
    adasdf::MeshDiagnosticsOptions diagnostics_options;
    adasdf::MeshReadinessOptions readiness_options;
    adasdf::MeshCleanupOptions cleanup_options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--merge-tolerance" && hasValue(i, argc)) {
        cleanup_options.vertex_merge_tolerance = std::stod(argv[++i]);
        read_options.vertex_merge_tolerance = cleanup_options.vertex_merge_tolerance;
        diagnostics_options.duplicate_triangle_tolerance =
            cleanup_options.vertex_merge_tolerance;
      } else if (arg == "--area-eps" && hasValue(i, argc)) {
        cleanup_options.degenerate_area_epsilon = std::stod(argv[++i]);
        diagnostics_options.degenerate_area_epsilon =
            cleanup_options.degenerate_area_epsilon;
      } else if (arg == "--no-merge-vertices") {
        cleanup_options.merge_near_duplicate_vertices = false;
      } else if (arg == "--no-remove-degenerate") {
        cleanup_options.remove_degenerate_triangles = false;
      } else if (arg == "--no-remove-duplicates") {
        cleanup_options.remove_duplicate_triangles = false;
      } else if (arg == "--no-remove-unused") {
        cleanup_options.remove_unused_vertices = false;
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
      return 1;
    }
    const auto input_abs = std::filesystem::absolute(input).lexically_normal();
    const auto output_abs = std::filesystem::absolute(output).lexically_normal();
    if (input_abs == output_abs) {
      std::cerr << "adasdf_mesh_clean: refusing to overwrite input STL\n";
      return 1;
    }

    const adasdf::STLReadResult read =
        adasdf::STLReader::read(input.string(), read_options);
    if (!read.success) {
      std::cerr << "adasdf_mesh_clean: failed to read STL: "
                << read.error_message << "\n";
      return 1;
    }

    adasdf::MeshDiagnosticsReport before_diag =
        adasdf::MeshDiagnostics::analyze(read.mesh, diagnostics_options);
    before_diag.raw_triangle_count = read.raw_triangle_count;
    const adasdf::MeshReadinessReport before_ready =
        adasdf::MeshReadiness::evaluate(before_diag, readiness_options);

    const adasdf::MeshCleanupResult cleanup =
        adasdf::MeshCleanup::clean(read.mesh, cleanup_options);
    if (!cleanup.success) {
      std::cerr << "adasdf_mesh_clean: cleanup failed: "
                << cleanup.error_message << "\n";
      return 1;
    }

    std::string write_error;
    adasdf::STLWriteOptions write_options;
    write_options.solid_name = "adasdf_cleaned_mesh";
    if (!adasdf::STLWriter::write(
            output.string(),
            cleanup.cleaned_mesh,
            write_options,
            &write_error)) {
      std::cerr << "adasdf_mesh_clean: failed to write cleaned STL: "
                << write_error << "\n";
      return 1;
    }

    adasdf::MeshDiagnosticsReport after_diag =
        adasdf::MeshDiagnostics::analyze(
            cleanup.cleaned_mesh,
            diagnostics_options);
    after_diag.raw_triangle_count = cleanup.cleaned_mesh.triangleCount();
    const adasdf::MeshReadinessReport after_ready =
        adasdf::MeshReadiness::evaluate(after_diag, readiness_options);

    printCleanup(cleanup.stats);
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Before readiness: " << adasdf::toString(before_ready.level)
              << ", score " << before_ready.score << "\n";
    std::cout << "After readiness: " << adasdf::toString(after_ready.level)
              << ", score " << after_ready.score << "\n";

    if (!report_path.empty()) {
      writeText(
          report_path,
          adasdf::MeshDiagnosticsWriter::cleanupComparisonMarkdown(
              before_diag,
              before_ready,
              cleanup.stats,
              after_diag,
              after_ready));
      std::cout << "Report: " << report_path.string() << "\n";
    }

    return (after_ready.level == adasdf::MeshReadinessLevel::Ready ||
            after_ready.level ==
                adasdf::MeshReadinessLevel::UsableWithWarnings)
        ? 0
        : 2;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_mesh_clean failed: " << exc.what() << "\n";
    return 1;
  }
}
