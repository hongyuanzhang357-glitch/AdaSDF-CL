#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
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
  const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  if (build_tool.empty()) {
    std::cout << "SKIP: compressed builder tool was not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto model = temp / "compressed_bvh_cli.sdfbin";
  const auto stdout_txt = temp / "compressed_bvh_cli_stdout.txt";
  const std::string command =
      executableCommand(build_tool) + " " + quote(fixture) + " " +
      quote(model) +
      " --max-level 1 --block-resolution 4 --unsigned --fixed-rank 2 "
      "--accel bvh --threads 2 --benchmark-brute-reference > " +
      quote(stdout_txt);
  if (std::system(command.c_str()) != 0 || !std::filesystem::exists(model)) {
    std::cerr << "compressed BVH builder CLI failed\n";
    return 1;
  }
  const std::string text = readFile(stdout_txt);
  if (!contains(text, "Acceleration: bvh") ||
      !contains(text, "Used BVH: yes") ||
      !contains(text, "BVH build time ms:")) {
    std::cerr << "compressed BVH CLI output missing acceleration stats\n";
    return 1;
  }
  std::cout << "compressed BVH builder CLI passed\n";
  return 0;
}
