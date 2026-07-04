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

  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.enable_counters = true;
  adasdf::BuildAccelerationStats stats;
  adasdf::BVHSDFSampler sampler;
  sampler.reset(read.mesh, sampler_options, &stats);

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.hierarchical_diagnostics = true;
  options.coarse_resolution = 2;
  options.quality_check_samples_per_axis = 2;
  options.transition_quality_check_samples_per_axis = 2;
  options.target_max_abs_error = 100.0;
  options.target_rms_error = 100.0;
  options.target_p95_error = 100.0;

  const adasdf::AABB far_bounds{{3.0, 3.0, 3.0}, {4.0, 4.0, 4.0}, true};
  const auto far = adasdf::HierarchicalBlockSampler::sampleBlock(
      far_bounds, 4, 4, 1, 4, true, false, sampler, options);
  if (!far.success || far.coarse_sample_count == 0 ||
      far.reused_coarse_sample_count != far.coarse_sample_count ||
      far.diagnostics.reused_coarse_sample_count != far.coarse_sample_count) {
    std::cerr << "coarse sample reuse was not reported\n";
    return 1;
  }

  const adasdf::AABB near_bounds{{-0.1, -0.1, -0.1}, {1.1, 1.1, 1.1}, true};
  const auto near = adasdf::HierarchicalBlockSampler::sampleBlock(
      near_bounds, 5, 5, 1, 4, true, true, sampler, options);
  if (!near.success || near.coarse_sample_count != 0 ||
      near.exact_sample_count != 64 ||
      near.diagnostics.coarse_sample_count != 0) {
    std::cerr << "near-surface exact path still sampled coarse probes\n";
    return 1;
  }

  std::cout << "hierarchical sampling coarse sample reuse passed\n";
  return 0;
}
