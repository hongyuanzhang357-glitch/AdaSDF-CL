#include <adasdf/adasdf.h>

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
    const auto closed = fixture_dir / "closed_cube_ascii.stl";
    const auto open = fixture_dir / "open_cube_missing_face_ascii.stl";
    const auto closed_txt = temp / "mesh_check_readiness_closed.txt";
    const auto open_txt = temp / "mesh_check_readiness_open.txt";
    const auto lenient_txt = temp / "mesh_check_readiness_lenient.txt";
    const auto md_path = temp / "mesh_check_readiness_closed.md";
    const auto json_path = temp / "mesh_check_readiness_closed.json";

    const std::string command_tool = executableCommand(tool);
    const std::string closed_command =
        command_tool + " " + quote(closed) + " --readiness --out " +
        quote(md_path) + " --json " + quote(json_path) + " > " +
        quote(closed_txt);
    if (std::system(closed_command.c_str()) != 0) {
      std::cerr << "mesh_check readiness closed cube command failed\n";
      return 1;
    }
    if (!contains(readFile(closed_txt), "SDF build readiness: Ready") ||
        !contains(readFile(closed_txt), "Score: 100 / 100") ||
        !contains(readFile(md_path), "SDF Build Readiness") ||
        !contains(readFile(md_path), "Score") ||
        !contains(readFile(json_path), "\"readiness\"") ||
        !contains(readFile(json_path), "\"score\"")) {
      std::cerr << "readiness closed cube output missing\n";
      return 1;
    }

    const std::string open_command =
        command_tool + " " + quote(open) + " --readiness --out " +
        quote(temp / "mesh_check_readiness_open.md") + " > " +
        quote(open_txt);
    if (std::system(open_command.c_str()) == 0) {
      std::cerr << "open cube readiness should report poor readiness\n";
      return 1;
    }
    if (!contains(readFile(open_txt), "SDF build readiness: Poor") ||
        !contains(readFile(open_txt), "BOUNDARY_EDGES")) {
      std::cerr << "open cube readiness output missing boundary issue\n";
      return 1;
    }

    const std::string lenient_command =
        command_tool + " " + quote(open) + " --readiness --lenient > " +
        quote(lenient_txt);
    if (std::system(lenient_command.c_str()) != 0) {
      std::cerr << "lenient open cube readiness should be usable\n";
      return 1;
    }
    if (!contains(readFile(lenient_txt), "SDF build readiness: UsableWithWarnings")) {
      std::cerr << "lenient readiness output missing expected level\n";
      return 1;
    }

    std::cout << "mesh check readiness CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_check_readiness_cli failed: " << exc.what()
              << "\n";
    return 1;
  }
}
