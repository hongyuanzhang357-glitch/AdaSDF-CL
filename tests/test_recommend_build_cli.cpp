#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_RECOMMEND_BUILD
#define ADASDF_CL_RECOMMEND_BUILD ""
#endif

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
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
  if (tool.find('\\') == std::string::npos && tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos && tool.find('\\') == std::string::npos) {
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
    const std::string recommend_tool = ADASDF_CL_RECOMMEND_BUILD;
    if (recommend_tool.empty()) {
      std::cout << "SKIP: adasdf_recommend_build was not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto markdown = temp / "recommend_build_cli.md";
    const auto json = temp / "recommend_build_cli.json";
    const auto stdout_path = temp / "recommend_build_cli_stdout.txt";
    const std::string command =
        executableCommand(recommend_tool) + " " +
        quote(fixture_dir / "closed_cube_ascii.stl") +
        " --target-error 1e-3 --memory-mb 256 --use-case contact "
        "--out " + quote(markdown) + " --json " + quote(json) +
        " --emit-command > " + quote(stdout_path);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(markdown) ||
        !std::filesystem::exists(json)) {
      std::cerr << "recommend build CLI failed\n";
      return 1;
    }
    const std::string stdout_text = readFile(stdout_path);
    if (!contains(stdout_text, "AdaSDF-CL build recommender") ||
        !contains(stdout_text, "Recommended path:") ||
        !contains(stdout_text, "adasdf_build_") ||
        !contains(readFile(markdown), "Recommended path") ||
        !contains(readFile(markdown), "CLI command") ||
        !contains(readFile(json), "recommended_recipe")) {
      std::cerr << "recommend build CLI output missing required content\n";
      return 1;
    }

    const std::string dense_tool = ADASDF_CL_BUILD_DENSE_SDF;
    if (!dense_tool.empty()) {
      const auto recommend_hint = temp / "recommend_hint.txt";
      const std::string hint_command =
          executableCommand(dense_tool) +
          " --recommend > " + quote(recommend_hint);
      if (std::system(hint_command.c_str()) != 0 ||
          !contains(readFile(recommend_hint), "adasdf_recommend_build")) {
        std::cerr << "build tool --recommend hint failed\n";
        return 1;
      }
    }
    std::cout << "recommend build CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_recommend_build_cli failed: " << exc.what() << "\n";
    return 1;
  }
}
