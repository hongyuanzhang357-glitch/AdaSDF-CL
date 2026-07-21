#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    if (!read.success) {
      std::cerr << "failed to read fixture\n";
      return 1;
    }
    adasdf::NearSurfaceSampleOptions options;
    options.surface_sample_count = 6;
    options.offsets = {-0.01, 0.0, 0.01};
    const adasdf::NearSurfaceSampleSet samples =
        adasdf::NearSurfaceSampleGenerator::generate(read.mesh, options);
    if (samples.generated_surface_sample_count != 6 ||
        samples.samples.size() != 18 || !(samples.total_surface_area > 0.0)) {
      std::cerr << "unexpected sample generator counts\n";
      return 1;
    }
    for (const adasdf::NearSurfaceSample& sample : samples.samples) {
      if (!sample.point.allFinite() || !sample.surface_point.allFinite() ||
          !sample.triangle_normal.allFinite() || sample.triangle_index < 0) {
        std::cerr << "sample contains invalid data\n";
        return 1;
      }
    }
    std::cout << "near-surface sample generator passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_near_surface_sample_generator failed: "
              << exc.what() << "\n";
    return 1;
  }
}
