#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_BENCHMARK_SPARSE_QUERY
#define ADASDF_CL_BENCHMARK_SPARSE_QUERY ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BUILD_DENSE_SDF).empty() ||
      std::string(ADASDF_CL_BENCHMARK_SPARSE_QUERY).empty()) {
    std::cout << "SKIP: required tools not built\n";
    return 0;
  }
  const std::filesystem::path temp = std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "backend_contract_sparse_bench";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube_dense.sdfbin";
  const auto json = temp / "sparse_bench.json";
  const auto fixture = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / "closed_cube_ascii.stl";
  const auto samples = std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) / "cube_sparse_samples.csv";
  if (runCommand(executableCommand(ADASDF_CL_BUILD_DENSE_SDF) + " " +
                 quotePath(fixture) + " " + quotePath(model) +
                 " --resolution 16 > " + quotePath(temp / "build.txt")) != 0) {
    return 1;
  }
  if (runCommand(executableCommand(ADASDF_CL_BENCHMARK_SPARSE_QUERY) + " " +
                 quotePath(model) + " " + quotePath(samples) +
                 " --repeat 1 --warmup 0 --json > " + quotePath(json)) != 0) {
    return 1;
  }
  const std::string text = readText(json);
  if (!containsText(text, "\"schema_id\": \"adasdf.benchmark.v1\"") ||
      !containsText(text, "\"sample_count\"") ||
      !containsText(text, "\"ns_per_query\"")) {
    std::cerr << "sparse benchmark JSON missing expected fields\n";
    return 1;
  }
  return 0;
}
