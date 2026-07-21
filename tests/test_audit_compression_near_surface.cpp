#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_AUDIT_COMPRESSION_NEAR_SURFACE
#define ADASDF_CL_AUDIT_COMPRESSION_NEAR_SURFACE ""
#endif
#ifndef ADASDF_CL_BUILD_ADAPTIVE_SDF
#define ADASDF_CL_BUILD_ADAPTIVE_SDF ""
#endif
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
  const std::string audit_tool = ADASDF_CL_AUDIT_COMPRESSION_NEAR_SURFACE;
  const std::string adaptive_tool = ADASDF_CL_BUILD_ADAPTIVE_SDF;
  const std::string compressed_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  if (audit_tool.empty() || adaptive_tool.empty() ||
      compressed_tool.empty()) {
    std::cout << "SKIP: compression near-surface audit tools were not built\n";
    return 0;
  }

  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "audit_compression_near_surface";
  std::filesystem::create_directories(temp);
  const auto dense = temp / "cube_adaptive.sdfbin";
  const auto compressed = temp / "cube_compressed.sdfbin";
  const auto json = temp / "audit.json";
  const auto csv = temp / "audit.csv";
  const auto report = temp / "audit.md";

  const int dense_rc = runCommand(
      executableCommand(adaptive_tool) + " " + quotePath(fixture) + " " +
      quotePath(dense) +
      " --target-error 1e-3 --max-level 1 --block-resolution 4 "
      "--accel bvh > " +
      quotePath(temp / "dense_stdout.txt"));
  if (dense_rc != 0 || !std::filesystem::exists(dense)) {
    std::cerr << "adaptive fixture build failed\n";
    return 1;
  }

  const int compressed_rc = runCommand(
      executableCommand(compressed_tool) + " " + quotePath(fixture) + " " +
      quotePath(compressed) +
      " --target-error 1e-3 --max-level 1 --block-resolution 4 "
      "--max-rank 4 --accel bvh > " +
      quotePath(temp / "compressed_stdout.txt"));
  if (compressed_rc != 0 || !std::filesystem::exists(compressed)) {
    std::cerr << "compressed fixture build failed\n";
    return 1;
  }

  const int audit_rc = runCommand(
      executableCommand(audit_tool) + " --dense " + quotePath(dense) +
      " --compressed " + quotePath(compressed) + " --stl " +
      quotePath(fixture) +
      " --near-band 0.05 --surface-samples 8 --offsets -0.01,0,0.01 "
      "--json " +
      quotePath(json) + " --csv " + quotePath(csv) + " --report " +
      quotePath(report) + " > " + quotePath(temp / "audit_stdout.txt"));
  if (audit_rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(csv) || !std::filesystem::exists(report)) {
    std::cerr << "compression near-surface audit CLI failed\n";
    return 1;
  }

  const std::string json_text = readText(json);
  const std::string report_text = readText(report);
  if (!containsText(json_text, "adasdf.compression_near_surface_audit.v1") ||
      !containsText(json_text, "compressed_dense_p95_abs_error") ||
      !containsText(json_text, "near_zero_compression_sign_flip_count") ||
      !containsText(report_text, "Compression Near-Surface Audit")) {
    std::cerr << "compression near-surface audit output missing fields\n";
    return 1;
  }

  std::cout << "audit compression near-surface passed\n";
  return 0;
}
