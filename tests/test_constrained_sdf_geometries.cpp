#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

adasdf::TriangleMesh makeOctahedralSphere() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {
      {0.0, 0.0, 1.0},
      {0.0, 0.0, -1.0},
      {1.0, 0.0, 0.0},
      {0.0, 1.0, 0.0},
      {-1.0, 0.0, 0.0},
      {0.0, -1.0, 0.0}};
  mesh.triangles = {
      {0, 2, 3, 0}, {0, 3, 4, 1}, {0, 4, 5, 2}, {0, 5, 2, 3},
      {1, 3, 2, 4}, {1, 4, 3, 5}, {1, 5, 4, 6}, {1, 2, 5, 7}};
  return mesh;
}

}  // namespace

int main() {
  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "constrained_sdf_geometries";
  std::filesystem::create_directories(dir);

  adasdf::SDFCreationConstraints constraints;
  constraints.max_sdf_file_bytes = 32ull * 1024ull * 1024ull;
  constraints.max_decoded_block_bytes = 256ull * 1024ull;
  constraints.max_zero_surface_abs_error = 0.3;
  const std::filesystem::path output = dir / "octahedral_sphere.sdfbin";
  const auto result = adasdf::ConstrainedSDFBuilder::fromMesh(
      makeOctahedralSphere(), output, constraints);
  if (!result.feasible() ||
      !result.report.actual.zero_surface_validation_complete ||
      result.report.actual.zero_surface_validation_sample_count < 18 ||
      result.report.actual.max_zero_surface_abs_error >
          constraints.max_zero_surface_abs_error) {
    std::cerr << adasdf::toJson(result.report) << "\n";
    return 1;
  }

  const auto reloaded = adasdf::SDFBinReader::read(output);
  if (!reloaded || reloaded->sampleDistance({0.0, 0.0, 0.0}) >= 0.0 ||
      reloaded->sampleDistance({2.0, 0.0, 0.0}) <= 0.0 ||
      std::abs(reloaded->sampleDistance({1.0, 0.0, 0.0})) >
          constraints.max_zero_surface_abs_error) {
    std::cerr << "reloaded sphere-like constrained SDF has wrong sign/error\n";
    return 1;
  }

  std::filesystem::remove_all(dir);
  return 0;
}
