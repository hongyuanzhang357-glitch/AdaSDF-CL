#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ADASDF_CL_CAPABILITY_WALKTHROUGH
#define ADASDF_CL_CAPABILITY_WALKTHROUGH ""
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
  const std::string tool = ADASDF_CL_CAPABILITY_WALKTHROUGH;
  if (tool.empty()) {
    std::cout << "SKIP: capability walkthrough example was not built\n";
    return 0;
  }

  const std::string output =
      std::string(ADASDF_CL_TEST_TEMP_DIR) + "/capability_walkthrough.txt";
  std::filesystem::create_directories(ADASDF_CL_TEST_TEMP_DIR);
  const std::string command = executableCommand(tool) + " > \"" + output + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    std::cerr << "capability walkthrough command failed\n";
    return 1;
  }

  const std::string text = readFile(output);
  const std::vector<std::string> required = {
      "AdaSDF-CL capability walkthrough",
      "Version:",
      "Point phi:",
      "Collision hit:",
      "Contact penetration depth:",
      "Expansion p95 abs error:",
      "Sign mismatch rate:",
      "Status: ok"};
  for (const std::string& needle : required) {
    if (!contains(text, needle)) {
      std::cerr << "walkthrough output missing: " << needle << "\n";
      return 1;
    }
  }

  std::cout << "capability walkthrough example passed\n";
  return 0;
}
