#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  if (!read.success) {
    std::cerr << "failed to read cube fixture\n";
    return 1;
  }

  adasdf::AdaptiveBlockSDFBuildOptions options;
  options.max_octree_level = 1;
  options.block_resolution = 4;
  options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  options.hierarchical_sampling.enable_hierarchical_sampling = true;
  options.hierarchical_sampling.hierarchical_diagnostics = true;
  options.hierarchical_sampling.coarse_resolution = 2;
  options.hierarchical_sampling.quality_check_samples_per_axis = 2;
  options.hierarchical_sampling.transition_quality_check_samples_per_axis = 2;
  options.hierarchical_sampling.target_max_abs_error = 10.0;
  options.hierarchical_sampling.target_rms_error = 10.0;
  options.hierarchical_sampling.target_p95_error = 10.0;

  adasdf::AdaptiveBlockSDFBuildReport report;
  const auto model =
      adasdf::AdaptiveBlockSDFBuilder::fromMesh(read.mesh, options, &report);
  if (!model || !report.success) {
    std::cerr << "hierarchical diagnostic build failed: "
              << report.error_message << "\n";
    return 1;
  }
  const auto& diagnostics = report.hierarchical_sampling.diagnostics;
  if (diagnostics.total_block_count != report.block_count ||
      diagnostics.fine_sample_count == 0 ||
      diagnostics.exact_bvh_sample_count == 0 ||
      !std::isfinite(diagnostics.exact_sample_reduction_ratio) ||
      !std::isfinite(diagnostics.prediction_acceptance_rate) ||
      !std::isfinite(diagnostics.fallback_rate)) {
    std::cerr << "diagnostics were not populated\n";
    return 1;
  }

  std::cout << "hierarchical sampling diagnostics passed\n";
  return 0;
}
