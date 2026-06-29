#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#ifndef ADASDF_CL_QUERY_MODE_DEMO
#define ADASDF_CL_QUERY_MODE_DEMO ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
  const std::filesystem::path path(tool);
  if (path.has_parent_path()) {
    return tool;
  }
#ifdef _WIN32
  return ".\\" + tool;
#else
  return "./" + tool;
#endif
}

bool runOk(const std::string& command) {
  return std::system(command.c_str()) == 0;
}

}  // namespace

int main() {
  const std::string tool = ADASDF_CL_QUERY_MODE_DEMO;
  if (tool.empty() || !std::filesystem::exists(tool)) {
    std::cerr << "adasdf_query_mode_demo executable is missing: " << tool << "\n";
    return 1;
  }
  const std::string exe = executableCommand(tool);

  if (!runOk(exe)) {
    std::cerr << "query mode demo usage invocation failed\n";
    return 1;
  }
  if (!runOk(exe + " --backend cpu --expansion none --points 100")) {
    std::cerr << "query mode demo CPU none failed\n";
    return 1;
  }
  if (!runOk(exe + " --backend cpu --expansion global --points 100")) {
    std::cerr << "query mode demo CPU global failed\n";
    return 1;
  }
  if (!runOk(exe + " --backend cpu --expansion block --blocks all --points 100")) {
    std::cerr << "query mode demo CPU block failed\n";
    return 1;
  }
  if (runOk(exe + " --backend cuda --expansion none --points 100")) {
    std::cerr << "query mode demo CUDA none unexpectedly succeeded\n";
    return 1;
  }

  std::cout << "query mode demo CLI smoke tests passed\n";
  return 0;
}
