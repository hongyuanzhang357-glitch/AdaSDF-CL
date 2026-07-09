#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BENCHMARK_BUILD_CACHE
#define ADASDF_CL_BENCHMARK_BUILD_CACHE ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BENCHMARK_BUILD_CACHE).empty()) {
    std::cout << "SKIP: build cache benchmark not built\n";
    return 0;
  }
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "benchmark_build_cache_cli";
  std::filesystem::create_directories(temp);
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto csv = temp / "build_cache.csv";
  const auto report = temp / "build_cache.md";
  const auto stdout_txt = temp / "stdout.txt";
  const std::string command =
      executableCommand(ADASDF_CL_BENCHMARK_BUILD_CACHE) + " " +
      quotePath(fixture) +
      " --builder compressed --max-level 1 --block-resolution 8 "
      "--target-error 1e-3 --max-rank 4 --sampling contact-band "
      "--contact-band-width 1e-3 --contact-band-marker distance-aware "
      "--marker-cell-size-factor 0.5 --marker-safety-factor 1.0 "
      "--local-halo-only --cache-modes off,block --distance-backend bvh "
      "--threads 1 --csv " +
      quotePath(csv) + " --report " + quotePath(report) +
      " --case-id build_cache_cli > " + quotePath(stdout_txt) + " 2>&1";
  if (runCommand(command) != 0 || !std::filesystem::exists(csv) ||
      !std::filesystem::exists(report)) {
    std::cerr << "build cache benchmark CLI failed\n";
    return 1;
  }
  const std::string stdout_text = readText(stdout_txt);
  const std::string csv_text = readText(csv);
  if (!containsText(stdout_text, "AdaSDF-CL build cache benchmark") ||
      !containsText(csv_text, "speedup_vs_no_cache") ||
      !containsText(csv_text, "distance_queries_saved") ||
      !containsText(csv_text, "quality_passed") ||
      !containsText(csv_text, "performance_claim_allowed")) {
    std::cerr << "build cache benchmark output missing required fields\n";
    return 1;
  }
  bool checked_block_quality = false;
  std::istringstream lines(csv_text);
  std::string line;
  while (std::getline(lines, line)) {
    if (line.find(",block,") == std::string::npos) {
      continue;
    }
    std::vector<std::string> fields;
    std::istringstream cells(line);
    std::string cell;
    while (std::getline(cells, cell, ',')) {
      fields.push_back(cell);
    }
    if (fields.size() < 27) {
      std::cerr << "build cache benchmark row has too few fields\n";
      return 1;
    }
    if (fields[20] != "0" || fields[23] != "0" ||
        fields[25] != "true") {
      std::cerr << "cache-on benchmark must match no-cache reference\n";
      return 1;
    }
    checked_block_quality = true;
  }
  if (!checked_block_quality) {
    std::cerr << "build cache benchmark did not produce a block-cache row\n";
    return 1;
  }
  return 0;
}
