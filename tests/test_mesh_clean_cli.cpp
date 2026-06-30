#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_MESH_CLEAN
#define ADASDF_CL_MESH_CLEAN ""
#endif

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
    const std::string clean_tool = ADASDF_CL_MESH_CLEAN;
    const std::string check_tool = ADASDF_CL_MESH_CHECK;
    if (clean_tool.empty() || check_tool.empty()) {
      std::cout << "SKIP: mesh cleanup tools were not built\n";
      return 0;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);

    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto input = fixture_dir / "closed_cube_ascii.stl";
    const auto cleaned = temp / "mesh_clean_cli_cleaned.stl";
    const auto report = temp / "mesh_clean_cli_report.md";
    const auto usage_txt = temp / "mesh_clean_usage.txt";
    const auto clean_txt = temp / "mesh_clean_output.txt";
    const auto check_txt = temp / "mesh_clean_check_output.txt";

    const std::string clean_cmd = executableCommand(clean_tool);
    if (std::system((clean_cmd + " > " + quote(usage_txt)).c_str()) != 0) {
      std::cerr << "mesh_clean usage command failed\n";
      return 1;
    }
    if (!contains(readFile(usage_txt), "Usage: adasdf_mesh_clean")) {
      std::cerr << "mesh_clean usage output missing\n";
      return 1;
    }

    const std::string command =
        clean_cmd + " " + quote(input) + " " + quote(cleaned) +
        " --report " + quote(report) + " > " + quote(clean_txt);
    if (std::system(command.c_str()) != 0) {
      std::cerr << "mesh_clean command failed\n";
      return 1;
    }
    if (!std::filesystem::exists(cleaned) ||
        !std::filesystem::exists(report) ||
        !contains(readFile(clean_txt), "Removed duplicate triangles:") ||
        !contains(readFile(report), "Before Diagnostics") ||
        !contains(readFile(report), "After Readiness")) {
      std::cerr << "mesh_clean output/report missing\n";
      return 1;
    }

    const std::string check_cmd =
        executableCommand(check_tool) + " " + quote(cleaned) +
        " --readiness > " + quote(check_txt);
    if (std::system(check_cmd.c_str()) != 0) {
      std::cerr << "mesh_check cleaned STL command failed\n";
      return 1;
    }
    if (!contains(readFile(check_txt), "SDF build readiness:")) {
      std::cerr << "mesh_check cleaned STL output missing readiness\n";
      return 1;
    }

    std::cout << "mesh clean CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_clean_cli failed: " << exc.what() << "\n";
    return 1;
  }
}
