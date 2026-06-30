#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_mesh_check model.stl [--out report.md] "
         "[--json report.json] [--tolerance 1e-12] [--area-eps 1e-14] "
         "[--no-duplicate-check] [--no-components] [--readiness] "
         "[--require-watertight] [--allow-open] [--strict] [--lenient] "
         "[--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void printVertex(const char* label, const adasdf::MeshVertex& vertex) {
  std::cout << label << vertex.x << " " << vertex.y << " " << vertex.z
            << "\n";
}

void printSummary(
    const std::filesystem::path& input,
    const adasdf::STLReadResult& read,
    const adasdf::MeshDiagnosticsReport& report,
    bool verbose) {
  std::cout << "AdaSDF-CL mesh diagnostics\n";
  std::cout << "Input: " << input.string() << "\n";
  std::cout << "Format: " << (read.is_binary ? "binary" : "ascii") << "\n";
  std::cout << "Vertices: " << report.vertex_count << "\n";
  std::cout << "Triangles: " << report.triangle_count << "\n";
  std::cout << "Raw triangles: " << report.raw_triangle_count << "\n";
  printVertex("AABB min: ", report.aabb.min);
  printVertex("AABB max: ", report.aabb.max);
  std::cout << "Diagonal: " << report.diagonal_length << "\n";
  std::cout << "Watertight: " << (report.watertight ? "yes" : "no") << "\n";
  std::cout << "Boundary edges: " << report.boundary_edge_count << "\n";
  std::cout << "Non-manifold edges: "
            << report.non_manifold_edge_count << "\n";
  std::cout << "Degenerate triangles: "
            << report.degenerate_triangle_count << "\n";
  std::cout << "Duplicate triangles: "
            << report.duplicate_triangle_count << "\n";
  std::cout << "Connected components: "
            << report.connected_component_count << "\n";
  std::cout << "Isolated vertices: " << report.isolated_vertex_count << "\n";
  std::cout << "Recommendation: " << report.recommendation << "\n";
  if (verbose) {
    for (const std::string& warning : report.warnings) {
      std::cout << "Warning: " << warning << "\n";
    }
    for (const std::string& error : report.errors) {
      std::cout << "Error: " << error << "\n";
    }
  }
}

void printIssueGroup(
    const adasdf::MeshReadinessReport& readiness,
    adasdf::MeshIssueSeverity severity,
    const char* label) {
  bool wrote = false;
  std::cout << label << ":\n";
  for (const adasdf::MeshIssue& issue : readiness.issues) {
    if (issue.severity == severity) {
      std::cout << "- " << issue.code << ": " << issue.message << "\n";
      std::cout << "  Suggestion: " << issue.suggestion << "\n";
      wrote = true;
    }
  }
  if (!wrote) {
    std::cout << "- none\n";
  }
}

void printReadiness(const adasdf::MeshReadinessReport& readiness) {
  std::cout << "SDF build readiness: " << adasdf::toString(readiness.level)
            << "\n";
  std::cout << "Score: " << readiness.score << " / 100\n";
  std::cout << "Recommended for SDF build: "
            << (readiness.recommended_for_sdf_build ? "yes" : "no")
            << "\n";
  std::cout << "Recommended for contact query: "
            << (readiness.recommended_for_contact_query ? "yes" : "no")
            << "\n";
  std::cout << "Readiness summary: " << readiness.summary << "\n";
  printIssueGroup(readiness, adasdf::MeshIssueSeverity::Critical, "Critical");
  printIssueGroup(readiness, adasdf::MeshIssueSeverity::Warning, "Warnings");
  std::cout << "Recommended steps:\n";
  for (std::size_t i = 0; i < readiness.recommended_steps.size(); ++i) {
    std::cout << (i + 1) << ". " << readiness.recommended_steps[i] << "\n";
  }
}

void writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  file << text;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path markdown_output;
    std::filesystem::path json_output;
    bool verbose = false;
    bool readiness_requested = false;

    adasdf::STLReadOptions read_options;
    adasdf::MeshDiagnosticsOptions diagnostics_options;
    adasdf::MeshReadinessOptions readiness_options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--out" && hasValue(i, argc)) {
        markdown_output = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_output = argv[++i];
      } else if (arg == "--tolerance" && hasValue(i, argc)) {
        read_options.vertex_merge_tolerance = std::stod(argv[++i]);
        diagnostics_options.duplicate_triangle_tolerance =
            read_options.vertex_merge_tolerance;
      } else if (arg == "--area-eps" && hasValue(i, argc)) {
        diagnostics_options.degenerate_area_epsilon = std::stod(argv[++i]);
      } else if (arg == "--no-duplicate-check") {
        diagnostics_options.check_duplicate_triangles = false;
      } else if (arg == "--no-components") {
        diagnostics_options.check_connected_components = false;
      } else if (arg == "--readiness") {
        readiness_requested = true;
      } else if (arg == "--require-watertight") {
        readiness_requested = true;
        readiness_options.require_watertight = true;
      } else if (arg == "--allow-open") {
        readiness_requested = true;
        readiness_options.require_watertight = false;
      } else if (arg == "--strict") {
        readiness_requested = true;
        readiness_options.require_watertight = true;
        readiness_options.allow_duplicate_triangles = false;
        readiness_options.allow_degenerate_triangles = false;
        readiness_options.allow_non_manifold_edges = false;
        readiness_options.max_boundary_edges_warning = 0;
        readiness_options.max_duplicate_triangles_warning = 0;
        readiness_options.max_degenerate_triangles_warning = 0;
      } else if (arg == "--lenient") {
        readiness_requested = true;
        readiness_options.require_watertight = false;
        readiness_options.allow_duplicate_triangles = true;
        readiness_options.allow_degenerate_triangles = true;
        readiness_options.allow_non_manifold_edges = false;
      } else if (arg == "--verbose") {
        verbose = true;
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
      return 1;
    }

    const adasdf::STLReadResult read =
        adasdf::STLReader::read(input.string(), read_options);
    if (!read.success) {
      std::cerr << "adasdf_mesh_check: failed to read STL: "
                << read.error_message << "\n";
      return 1;
    }

    adasdf::MeshDiagnosticsReport report =
        adasdf::MeshDiagnostics::analyze(read.mesh, diagnostics_options);
    report.raw_triangle_count = read.raw_triangle_count;

    printSummary(input, read, report, verbose);

    adasdf::MeshReadinessReport readiness;
    if (readiness_requested) {
      readiness = adasdf::MeshReadiness::evaluate(report, readiness_options);
      printReadiness(readiness);
    }

    if (!markdown_output.empty()) {
      if (readiness_requested) {
        writeText(
            markdown_output,
            adasdf::MeshDiagnosticsWriter::combinedMarkdown(
                report,
                readiness));
      } else {
        adasdf::MeshDiagnosticsWriter::writeMarkdown(
            markdown_output.string(),
            report);
      }
      std::cout << "Markdown report: " << markdown_output.string() << "\n";
    }
    if (!json_output.empty()) {
      if (readiness_requested) {
        writeText(
            json_output,
            adasdf::MeshDiagnosticsWriter::combinedJson(report, readiness));
      } else {
        adasdf::MeshDiagnosticsWriter::writeJson(json_output.string(), report);
      }
      std::cout << "JSON report: " << json_output.string() << "\n";
    }

    if (readiness_requested) {
      return (readiness.level == adasdf::MeshReadinessLevel::Ready ||
              readiness.level ==
                  adasdf::MeshReadinessLevel::UsableWithWarnings)
          ? 0
          : 2;
    }

    const bool critical =
        !report.errors.empty() || report.boundary_edge_count > 0 ||
        report.non_manifold_edge_count > 0 ||
        report.degenerate_triangle_count > 0;
    return critical ? 2 : 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_mesh_check failed: " << exc.what() << "\n";
    return 1;
  }
}
