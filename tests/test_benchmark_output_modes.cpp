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

  const std::string out_phi = std::string(ADASDF_CL_TEST_TEMP_DIR) +
                              "/benchmark_output_phi.csv";
  const std::string cmd_phi =
      executableCommand(tool) +
      " --points 1000 --query-backend cpu --expansion none "
      "--output phi --out \"" + out_phi + "\"";
  if (std::system(cmd_phi.c_str()) != 0) {
    std::cerr << "benchmark --output phi failed\n";
    return 1;
  }

  const std::string phi_csv = readFile(out_phi);
  if (!contains(phi_csv, "output_mode") ||
      !contains(phi_csv, "phi,true") ||
      !contains(phi_csv, "correctness_checked")) {
    std::cerr << "output phi CSV fields are missing\n";
    return 1;
  }

  const std::string out_full = std::string(ADASDF_CL_TEST_TEMP_DIR) +
                               "/benchmark_output_full.csv";
  const std::string cmd_full =
      executableCommand(tool) +
      " --points 1000 --query-backend cpu --expansion none "
      "--output phi,normal --out \"" + out_full + "\"";
  if (std::system(cmd_full.c_str()) != 0) {
    std::cerr << "benchmark --output phi,normal failed\n";
    return 1;
  }

  const std::string full_csv = readFile(out_full);
  if (!contains(full_csv, "\"phi,normal\"") ||
      !contains(full_csv, ",false,")) {
    std::cerr << "output phi,normal row was not recorded\n";
    return 1;
  }

  const std::string out_alias = std::string(ADASDF_CL_TEST_TEMP_DIR) +
                                "/benchmark_output_phi_alias.csv";
  const std::string cmd_alias =
      executableCommand(tool) +
      " --points 1000 --query-backend cpu --expansion none "
      "--phi-only --out \"" + out_alias + "\"";
  if (std::system(cmd_alias.c_str()) != 0) {
    std::cerr << "benchmark --phi-only alias failed\n";
    return 1;
  }
  if (!contains(readFile(out_alias), "phi,true")) {
    std::cerr << "phi-only alias did not map to output phi\n";
    return 1;
  }

  std::cout << "benchmark output modes are supported\n";
  return 0;
}
