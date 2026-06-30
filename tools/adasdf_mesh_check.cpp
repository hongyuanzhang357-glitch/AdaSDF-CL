#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_mesh_check model.stl [--out report.md] "
         "[--json report.json] [--tolerance 1e-12] [--area-eps 1e-14] "
         "[--no-duplicate-check] [--no-components] [--verbose]\n";
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

    adasdf::STLReadOptions read_options;
    adasdf::MeshDiagnosticsOptions diagnostics_options;

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

    if (!markdown_output.empty()) {
      adasdf::MeshDiagnosticsWriter::writeMarkdown(
          markdown_output.string(),
          report);
      std::cout << "Markdown report: " << markdown_output.string() << "\n";
    }
    if (!json_output.empty()) {
      adasdf::MeshDiagnosticsWriter::writeJson(json_output.string(), report);
      std::cout << "JSON report: " << json_output.string() << "\n";
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
