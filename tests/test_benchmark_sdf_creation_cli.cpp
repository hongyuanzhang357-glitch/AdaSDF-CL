#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BENCHMARK_SDF_CREATION
#define ADASDF_CL_BENCHMARK_SDF_CREATION ""
#endif

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BENCHMARK_SDF_CREATION).empty()) {
    return 0;
  }
  const std::filesystem::path input =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "benchmark_sdf_creation";
  const std::filesystem::path csv = dir / "benchmark.csv";
  std::filesystem::create_directories(dir);
  const std::string command =
      executableCommand(ADASDF_CL_BENCHMARK_SDF_CREATION) + " " +
      quotePath(input) + " --work-dir " + quotePath(dir / "work") +
      " --max-sdf-file-bytes 67108864 --max-decoded-block-bytes 262144 "
      "--max-zero-surface-abs-error 0.15 --repeat 1 --csv " +
      quotePath(csv);
  if (runCommand(command) != 0 || !std::filesystem::exists(csv)) {
    std::cerr << "SDF creation benchmark smoke failed\n";
    return 1;
  }
  const std::string text = readText(csv);
  if (text.find(
          "repeat,wall_time_ms,status,library_version,mesh_hash,advisor_source")
          != 0 ||
      text.find("query_time_ms") == std::string::npos ||
      text.find("query_ns_per_sample") == std::string::npos ||
      text.find("Feasible") == std::string::npos) {
    std::cerr << "SDF creation benchmark fields are incomplete\n";
    return 1;
  }
  std::filesystem::remove_all(dir);
  return 0;
}
