#include <adasdf/adasdf.h>

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
  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  sampler.reset(read.mesh, sampler_options, &stats);

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.near_surface_mode = adasdf::NearSurfaceSamplingMode::Banded;
  options.near_surface_band_factor = 0.1;
  options.halo_exact_layers = 1;
  options.near_surface_check_samples_per_axis = 2;
  options.coarse_resolution = 3;
  options.target_max_abs_error = 100.0;
  options.target_rms_error = 100.0;
  options.target_p95_error = 100.0;

  const adasdf::AABB bounds{{0.2, 0.2, 0.2}, {0.8, 0.8, 0.8}, true};
  const auto sampled = adasdf::HierarchicalBlockSampler::sampleBlock(
      bounds, 9, 9, 1, 4, true, true, sampler, options);
  if (!sampled.success || !sampled.used_prediction ||
      sampled.diagnostics.near_surface_banded_block_count != 1 ||
      sampled.diagnostics.near_surface_local_exact_node_count == 0 ||
      sampled.diagnostics.near_surface_predicted_node_count == 0 ||
      sampled.quality.sign_mismatch_count != 0 ||
      sampled.quality.near_surface_sign_mismatch_count != 0) {
    std::cerr << "success=" << sampled.success
              << " used_prediction=" << sampled.used_prediction
              << " banded_blocks="
              << sampled.diagnostics.near_surface_banded_block_count
              << " local_exact="
              << sampled.diagnostics.near_surface_local_exact_node_count
              << " predicted="
              << sampled.diagnostics.near_surface_predicted_node_count
              << " sign_mismatch=" << sampled.quality.sign_mismatch_count
              << " near_sign_mismatch="
              << sampled.quality.near_surface_sign_mismatch_count << "\n";
    std::cerr << "near-surface banded prediction did not pass expected guards\n";
    return 1;
  }
  std::cout << "near-surface banded prediction passed\n";
  return 0;
}
