#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BENCHMARK_BATCH_QUERY
#define ADASDF_CL_BENCHMARK_BATCH_QUERY ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

std::string readFile(const std::string& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

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

}  // namespace

int main() {
  const std::string tool = ADASDF_CL_BENCHMARK_BATCH_QUERY;
  if (tool.empty()) {
    std::cout << "SKIPPED: benchmark executable was not built\n";
    return 0;
  }

  const std::string out = std::string(ADASDF_CL_TEST_TEMP_DIR) +
                          "/benchmark_device_only.csv";
  const std::string command =
      executableCommand(tool) +
      " --points 1000 --query-backend cuda --expansion global "
      "--output phi --device-only --kernel-only --reuse-resident "
      "--warmup 1 --repeat 2 --out \"" + out + "\"";
  if (std::system(command.c_str()) != 0) {
    std::cerr << "device-only benchmark command failed\n";
    return 1;
  }

  const std::string csv = readFile(out);
  if (!contains(csv, "download_results") ||
      !contains(csv, "correctness_checked") ||
      !contains(csv, "false,false")) {
    std::cerr << "device-only CSV did not mark no-download semantics\n";
    return 1;
  }

  std::cout << "device-only benchmark mode is available or gracefully skipped\n";
  return 0;
}
