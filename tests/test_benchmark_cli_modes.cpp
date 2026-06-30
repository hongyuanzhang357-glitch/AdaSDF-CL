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
                          "/benchmark_cli_modes.csv";
  const std::string command =
      executableCommand(tool) +
      " --points 64 --query-backend cpu --expansion none "
      "--repeat 2 --phi-only --out \"" + out + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    std::cerr << "benchmark repeat/phi-only command failed\n";
    return 1;
  }

  const std::string csv = readFile(out);
  if (!contains(csv, "repeat") || !contains(csv, "total_min_ms") ||
      !contains(csv, "total_max_ms") || !contains(csv, "total_std_ms")) {
    std::cerr << "repeat statistics fields are missing\n";
    return 1;
  }
  if (!contains(csv, ",2,") || !contains(csv, "true,false,false,ok")) {
    std::cerr << "repeat or phi-only flags were not recorded\n";
    return 1;
  }
  if (!contains(csv, ",NA,")) {
    std::cerr << "phi-only CPU row did not mark a non-applicable field as NA\n";
    return 1;
  }

  std::cout << "benchmark CLI modes generated expected CSV fields\n";
  return 0;
}
