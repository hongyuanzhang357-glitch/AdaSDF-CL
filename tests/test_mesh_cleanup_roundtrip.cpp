#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_MESH_CHECK
#define ADASDF_CL_MESH_CHECK ""
#endif

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos &&
      tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    const std::string tool = ADASDF_CL_MESH_CHECK;
    if (tool.empty()) {
      std::cout << "SKIP: adasdf_mesh_check was not built\n";
      return 0;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);

    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto input = fixture_dir / "duplicate_and_degenerate_ascii.stl";
    const auto cleaned = temp / "mesh_check_clean_out.stl";
    const auto report = temp / "mesh_check_clean_report.md";
    const auto stdout_path = temp / "mesh_check_clean_stdout.txt";

    const std::string command =
        executableCommand(tool) + " " + quote(input) +
        " --readiness --clean-out " + quote(cleaned) +
        " --clean-report " + quote(report) + " > " + quote(stdout_path);
    (void)std::system(command.c_str());

    const std::string stdout_text = readFile(stdout_path);
    const std::string report_text = readFile(report);
    if (!std::filesystem::exists(cleaned) ||
        !contains(stdout_text, "After readiness:") ||
        !contains(stdout_text, "removed duplicate triangles:") ||
        !contains(report_text, "Cleanup Operations") ||
        !contains(report_text, "Removed degenerate triangles")) {
      std::cerr << "mesh_check cleanup roundtrip output missing\n";
      return 1;
    }

    std::cout << "mesh cleanup roundtrip passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_cleanup_roundtrip failed: " << exc.what()
              << "\n";
    return 1;
  }
}
