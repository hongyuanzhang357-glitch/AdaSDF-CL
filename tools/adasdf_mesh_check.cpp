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
      << "Usage: adasdf_mesh_check model.stl [--out report.md] "
         "[--json report.json] [--tolerance 1e-12] [--area-eps 1e-14] "
         "[--no-duplicate-check] [--no-components] [--readiness] "
         "[--require-watertight] [--allow-open] [--strict] [--lenient] "
         "[--clean-out cleaned.stl] [--clean-report cleanup_report.md] "
         "[--strict-json report.json] [--case-id case_id] "
         "[--merge-tolerance 1e-12] [--no-merge-vertices] "
         "[--no-remove-degenerate] [--no-remove-duplicates] "
         "[--no-remove-unused] "
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

void printCleanup(const adasdf::MeshCleanupStats& stats) {
  std::cout << "Cleanup:\n";
  std::cout << "  input vertices: " << stats.input_vertices << "\n";
  std::cout << "  input triangles: " << stats.input_triangles << "\n";
  std::cout << "  output vertices: " << stats.output_vertices << "\n";
  std::cout << "  output triangles: " << stats.output_triangles << "\n";
  std::cout << "  merged vertices: " << stats.merged_vertices << "\n";
  std::cout << "  removed degenerate triangles: "
            << stats.removed_degenerate_triangles << "\n";
  std::cout << "  removed duplicate triangles: "
            << stats.removed_duplicate_triangles << "\n";
  std::cout << "  removed unused vertices: " << stats.removed_unused_vertices
            << "\n";
  std::cout << "  topology may have changed: "
            << (stats.topology_may_have_changed ? "yes" : "no") << "\n";
  for (const std::string& warning : stats.warnings) {
    std::cout << "Cleanup warning: " << warning << "\n";
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
    std::filesystem::path clean_output;
    std::filesystem::path clean_report_output;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
    bool verbose = false;
    bool readiness_requested = false;
    const auto strict_timer = adasdf::startStrictRunTimer();
    std::map<std::string, std::string> strict_parameters =
        adasdf::commandLineParameters(argc, argv);
    auto strict_output_path = [&]() -> std::filesystem::path {
      if (!clean_output.empty()) {
        return clean_output;
      }
      if (!json_output.empty()) {
        return json_output;
      }
      return markdown_output;
    };
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
                  "adasdf_mesh_check",
                  case_id,
                  input,
                  strict_output_path(),
                  strict_parameters,
                  metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr << "adasdf_mesh_check: failed to write strict JSON: "
                      << strict_error << "\n";
          }
        };

    adasdf::STLReadOptions read_options;
    adasdf::MeshDiagnosticsOptions diagnostics_options;
    adasdf::MeshReadinessOptions readiness_options;
    adasdf::MeshCleanupOptions cleanup_options;

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
        cleanup_options.vertex_merge_tolerance =
            read_options.vertex_merge_tolerance;
      } else if (arg == "--area-eps" && hasValue(i, argc)) {
        diagnostics_options.degenerate_area_epsilon = std::stod(argv[++i]);
        cleanup_options.degenerate_area_epsilon =
            diagnostics_options.degenerate_area_epsilon;
      } else if (arg == "--merge-tolerance" && hasValue(i, argc)) {
        cleanup_options.vertex_merge_tolerance = std::stod(argv[++i]);
      } else if (arg == "--clean-out" && hasValue(i, argc)) {
        clean_output = argv[++i];
        readiness_requested = true;
      } else if (arg == "--clean-report" && hasValue(i, argc)) {
        clean_report_output = argv[++i];
        readiness_requested = true;
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (arg == "--no-merge-vertices") {
        cleanup_options.merge_near_duplicate_vertices = false;
      } else if (arg == "--no-remove-degenerate") {
        cleanup_options.remove_degenerate_triangles = false;
      } else if (arg == "--no-remove-duplicates") {
        cleanup_options.remove_duplicate_triangles = false;
      } else if (arg == "--no-remove-unused") {
        cleanup_options.remove_unused_vertices = false;
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
      write_strict(false, "failed", "missing input path");
      return 1;
    }

    const adasdf::STLReadResult read =
        adasdf::STLReader::read(input.string(), read_options);
    if (!read.success) {
      std::cerr << "adasdf_mesh_check: failed to read STL: "
                << read.error_message << "\n";
      write_strict(false, "failed", read.error_message);
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

    const bool cleanup_requested =
        !clean_output.empty() || !clean_report_output.empty();
    if (cleanup_requested) {
      if (!clean_output.empty()) {
        const auto input_abs = std::filesystem::absolute(input).lexically_normal();
        const auto output_abs =
            std::filesystem::absolute(clean_output).lexically_normal();
        if (input_abs == output_abs) {
          std::cerr << "adasdf_mesh_check: refusing to overwrite input STL\n";
          write_strict(false, "failed", "refusing to overwrite input STL");
          return 1;
        }
      }

      const adasdf::MeshCleanupResult cleanup =
          adasdf::MeshCleanup::clean(read.mesh, cleanup_options);
      if (!cleanup.success) {
        std::cerr << "adasdf_mesh_check: cleanup failed: "
                  << cleanup.error_message << "\n";
        write_strict(false, "failed", cleanup.error_message);
        return 1;
      }
      printCleanup(cleanup.stats);

      if (!clean_output.empty()) {
        std::string write_error;
        adasdf::STLWriteOptions write_options;
        write_options.solid_name = "adasdf_cleaned_mesh";
        if (!adasdf::STLWriter::write(
                clean_output.string(),
                cleanup.cleaned_mesh,
                write_options,
                &write_error)) {
          std::cerr << "adasdf_mesh_check: failed to write cleaned STL: "
                    << write_error << "\n";
          write_strict(false, "failed", write_error);
          return 1;
        }
        std::cout << "Output: " << clean_output.string() << "\n";
      }

      adasdf::MeshDiagnosticsReport after_report =
          adasdf::MeshDiagnostics::analyze(
              cleanup.cleaned_mesh,
              diagnostics_options);
      after_report.raw_triangle_count = cleanup.cleaned_mesh.triangleCount();
      const adasdf::MeshReadinessReport after_readiness =
          adasdf::MeshReadiness::evaluate(after_report, readiness_options);
      std::cout << "Before readiness: " << adasdf::toString(readiness.level)
                << ", score " << readiness.score << "\n";
      std::cout << "After readiness: " << adasdf::toString(after_readiness.level)
                << ", score " << after_readiness.score << "\n";

      if (!clean_report_output.empty()) {
        writeText(
            clean_report_output,
            adasdf::MeshDiagnosticsWriter::cleanupComparisonMarkdown(
                report,
                readiness,
                cleanup.stats,
                after_report,
                after_readiness));
        std::cout << "Report: " << clean_report_output.string() << "\n";
      }

      const int code =
          (after_readiness.level == adasdf::MeshReadinessLevel::Ready ||
           after_readiness.level ==
               adasdf::MeshReadinessLevel::UsableWithWarnings)
          ? 0
          : 2;
      write_strict(
          code == 0,
          code == 0 ? "ok" : "failed",
          code == 0 ? "" : "mesh not ready after cleanup",
          {{"triangle_count", static_cast<double>(after_report.triangle_count)},
           {"vertex_count", static_cast<double>(after_report.vertex_count)},
           {"readiness_score", static_cast<double>(after_readiness.score)}});
      return code;
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
      const int code =
          (readiness.level == adasdf::MeshReadinessLevel::Ready ||
           readiness.level == adasdf::MeshReadinessLevel::UsableWithWarnings)
          ? 0
          : 2;
      write_strict(
          code == 0,
          code == 0 ? "ok" : "failed",
          code == 0 ? "" : "mesh readiness failed",
          {{"triangle_count", static_cast<double>(report.triangle_count)},
           {"vertex_count", static_cast<double>(report.vertex_count)},
           {"readiness_score", static_cast<double>(readiness.score)}});
      return code;
    }

    const bool critical =
        !report.errors.empty() || report.boundary_edge_count > 0 ||
        report.non_manifold_edge_count > 0 ||
        report.degenerate_triangle_count > 0;
    const int code = critical ? 2 : 0;
    write_strict(
        code == 0,
        code == 0 ? "ok" : "failed",
        code == 0 ? "" : "mesh diagnostics found critical issues",
        {{"triangle_count", static_cast<double>(report.triangle_count)},
         {"vertex_count", static_cast<double>(report.vertex_count)}});
    return code;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_mesh_check failed: " << exc.what() << "\n";
    return 1;
  }
}
