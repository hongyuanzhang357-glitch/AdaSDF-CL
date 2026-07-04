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
  if (!sampler.reset(read.mesh, sampler_options, &stats)) {
    std::cerr << "failed to reset BVH sampler\n";
    return 1;
  }

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.coarse_resolution = 2;
  options.quality_check_samples_per_axis = 2;
  options.target_max_abs_error = 10.0;
  options.target_rms_error = 10.0;
  options.target_p95_error = 10.0;
  const adasdf::AABB bounds{{-0.5, -0.5, -0.5}, {1.5, 1.5, 1.5}, true};
  const auto result = adasdf::HierarchicalBlockSampler::sampleBlock(
      bounds,
      7,
      11,
      1,
      4,
      true,
      false,
      sampler,
      options);
  if (!result.success || result.block.phi.size() != 64) {
    std::cerr << "hierarchical block sampling failed\n";
    return 1;
  }
  if (result.exact_sample_count == 0) {
    std::cerr << "quality guard should perform exact samples\n";
    return 1;
  }

  std::cout << "hierarchical block sampler passed\n";
  return 0;
}
