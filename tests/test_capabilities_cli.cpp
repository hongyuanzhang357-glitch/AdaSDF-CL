#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_CAPABILITIES
#define ADASDF_CL_CAPABILITIES ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  return tool;
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
  return "\"" + tool + "\"";
#endif
}

std::string readFile(const std::string& path) {
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
  const std::string tool = ADASDF_CL_CAPABILITIES;
  if (tool.empty()) {
    std::cout << "SKIP: adasdf_capabilities was not built\n";
    return 0;
  }

  const std::string base = std::string(ADASDF_CL_TEST_TEMP_DIR) + "/capabilities";
  std::filesystem::create_directories(ADASDF_CL_TEST_TEMP_DIR);
  const std::string normal = base + "_normal.txt";
  const std::string verbose = base + "_verbose.txt";
  const std::string command =
      executableCommand(tool) + " > \"" + normal + "\" && " +
      executableCommand(tool) + " --verbose > \"" + verbose + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    std::cerr << "adasdf_capabilities command failed\n";
    return 1;
  }

  const std::string normal_text = readFile(normal);
  const std::string verbose_text = readFile(verbose);
  if (!contains(normal_text, "AdaSDF-CL version:") ||
      !contains(normal_text, "Implemented:") ||
      !contains(normal_text, "Planned:")) {
    std::cerr << "normal capability output is missing required sections\n";
    return 1;
  }
  if (!contains(verbose_text, "Query backend matrix:") ||
      !contains(verbose_text, "docs/capability_matrix.md") ||
      !contains(verbose_text, "not a drop-in FCL replacement")) {
    std::cerr << "verbose capability output is missing required sections\n";
    return 1;
  }

  std::cout << "capabilities CLI passed\n";
  return 0;
}
