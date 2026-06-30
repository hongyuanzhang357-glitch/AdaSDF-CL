#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

void printReadiness(const char* label, const adasdf::MeshReadinessReport& report) {
  std::cout << label << ": " << adasdf::toString(report.level)
            << ", score " << report.score << " / 100\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      std::cout << "Usage: adasdf_mesh_cleanup_demo input.stl output_cleaned.stl\n";
      return 0;
    }

    const std::string input = argv[1];
    const std::string output = argv[2];

    const adasdf::STLReadResult read = adasdf::STLReader::read(input);
    if (!read.success) {
      std::cerr << "Failed to read STL: " << read.error_message << "\n";
      return 1;
    }

    auto before_diag = adasdf::MeshDiagnostics::analyze(read.mesh);
    before_diag.raw_triangle_count = read.raw_triangle_count;
    const auto before_ready = adasdf::MeshReadiness::evaluate(before_diag);
    printReadiness("Before readiness", before_ready);

    const adasdf::MeshCleanupResult cleanup =
        adasdf::MeshCleanup::clean(read.mesh);
    if (!cleanup.success) {
      std::cerr << "Cleanup failed: " << cleanup.error_message << "\n";
      return 1;
    }

    std::string write_error;
    if (!adasdf::STLWriter::write(output, cleanup.cleaned_mesh, {}, &write_error)) {
      std::cerr << "Failed to write cleaned STL: " << write_error << "\n";
      return 1;
    }

    auto after_diag = adasdf::MeshDiagnostics::analyze(cleanup.cleaned_mesh);
    after_diag.raw_triangle_count = cleanup.cleaned_mesh.triangleCount();
    const auto after_ready = adasdf::MeshReadiness::evaluate(after_diag);

    std::cout << "Removed degenerate triangles: "
              << cleanup.stats.removed_degenerate_triangles << "\n";
    std::cout << "Removed duplicate triangles: "
              << cleanup.stats.removed_duplicate_triangles << "\n";
    std::cout << "Removed unused vertices: "
              << cleanup.stats.removed_unused_vertices << "\n";
    printReadiness("After readiness", after_ready);
    std::cout << "Cleaned STL: " << output << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_mesh_cleanup_demo failed: " << exc.what() << "\n";
    return 1;
  }
}
