#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BUILD_COMPRESSED_SDF).empty()) {
    std::cout << "SKIP: compressed builder not built\n";
    return 0;
  }
  const std::filesystem::path temp = std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "distance_backend_selector";
  std::filesystem::create_directories(temp);
  const auto fixture = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / "closed_cube_ascii.stl";
  const auto bvh = temp / "bvh.sdfbin";
  const auto brute = temp / "brute.sdfbin";
  if (runCommand(executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
                 quotePath(fixture) + " " + quotePath(bvh) +
                 " --max-level 1 --block-resolution 4 --distance-backend bvh --threads auto > " +
                 quotePath(temp / "bvh.txt")) != 0) {
    return 1;
  }
  if (runCommand(executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
                 quotePath(fixture) + " " + quotePath(brute) +
                 " --max-level 1 --block-resolution 4 --distance-backend brute_force > " +
                 quotePath(temp / "brute.txt")) != 0) {
    return 1;
  }
  if (!containsText(readText(temp / "bvh.txt"), "Acceleration: bvh") ||
      !containsText(readText(temp / "brute.txt"), "Acceleration: brute")) {
    std::cerr << "distance backend selector did not reach expected modes\n";
    return 1;
  }
  return 0;
}
