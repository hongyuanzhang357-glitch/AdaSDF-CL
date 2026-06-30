#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_mesh_diagnostics_demo model.stl [report.md]\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      usage();
      return 0;
    }

    const std::filesystem::path input = argv[1];
    const std::filesystem::path report_path = argc >= 3 ? argv[2] : "";
    const adasdf::STLReadResult read = adasdf::STLReader::read(input.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }

    adasdf::MeshDiagnosticsReport report =
        adasdf::MeshDiagnostics::analyze(read.mesh);
    report.raw_triangle_count = read.raw_triangle_count;

    std::cout << "AdaSDF-CL mesh diagnostics demo\n";
    std::cout << "Version: " << adasdf::versionString() << "\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Format: " << (read.is_binary ? "binary" : "ascii") << "\n";
    std::cout << "Vertices: " << report.vertex_count << "\n";
    std::cout << "Triangles: " << report.triangle_count << "\n";
    std::cout << "Watertight: " << (report.watertight ? "yes" : "no")
              << "\n";
    std::cout << "Boundary edges: " << report.boundary_edge_count << "\n";
    std::cout << "Non-manifold edges: " << report.non_manifold_edge_count
              << "\n";
    std::cout << "Degenerate triangles: "
              << report.degenerate_triangle_count << "\n";
    std::cout << "Duplicate triangles: " << report.duplicate_triangle_count
              << "\n";
    std::cout << "Connected components: "
              << report.connected_component_count << "\n";
    std::cout << "Recommendation: " << report.recommendation << "\n";

    if (!report_path.empty()) {
      adasdf::MeshDiagnosticsWriter::writeMarkdown(
          report_path.string(),
          report);
      std::cout << "Report: " << report_path.string() << "\n";
    }

    std::cout << "Status: ok\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "mesh diagnostics demo failed: " << exc.what() << "\n";
    return 1;
  }
}
