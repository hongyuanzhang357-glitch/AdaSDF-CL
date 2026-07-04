#include <cstdlib>
#include <exception>
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
  try {
    const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
    if (build_tool.empty()) {
      std::cout << "SKIP: compressed builder was not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const auto output = temp / "compressed_hierarchical_cli.sdfbin";
    const auto stdout_txt = temp / "compressed_hierarchical_cli.txt";
    const std::string command =
        executableCommand(build_tool) + " " + quote(fixture) + " " +
        quote(output) +
        " --sampling hierarchical --max-level 1 --block-resolution 4 "
        "--coarse-resolution 2 --quality-check-samples 2 "
        "--target-sampling-error 10 --max-rank 4 > " + quote(stdout_txt);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(output)) {
      std::cerr << "hierarchical compressed builder CLI failed\n";
      return 1;
    }
    const std::string stdout_text = readFile(stdout_txt);
    if (!contains(stdout_text, "Sampling mode: hierarchical") ||
        !contains(stdout_text, "Hierarchical")) {
      std::cerr << "hierarchical CLI output missing sampling fields\n";
      return 1;
    }

    std::cout << "compressed builder hierarchical sampling CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compressed_builder_hierarchical_sampling_cli failed: "
              << exc.what() << "\n";
    return 1;
  }
}
