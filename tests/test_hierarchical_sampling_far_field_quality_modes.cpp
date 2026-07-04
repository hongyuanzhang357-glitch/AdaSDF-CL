#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

adasdf::HierarchicalBlockSamplingResult sampleFar(
    adasdf::FarFieldQualityCheckMode mode) {
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.enable_counters = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  sampler.reset(read.mesh, sampler_options, &stats);

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.hierarchical_diagnostics = true;
  options.coarse_resolution = 2;
  options.quality_check_samples_per_axis = 3;
  options.transition_quality_check_samples_per_axis = 2;
  options.far_field_quality_check = mode;
  options.target_max_abs_error = 100.0;
  options.target_rms_error = 100.0;
  options.target_p95_error = 100.0;
  const adasdf::AABB bounds{{3.0, 3.0, 3.0}, {4.0, 4.0, 4.0}, true};
  return adasdf::HierarchicalBlockSampler::sampleBlock(
      bounds, 3, 3, 1, 4, true, false, sampler, options);
}

}  // namespace

int main() {
  const auto corners = sampleFar(adasdf::FarFieldQualityCheckMode::Corners);
  const auto full = sampleFar(adasdf::FarFieldQualityCheckMode::Full);
  if (!corners.success || !full.success) {
    std::cerr << "far-field sampling failed\n";
    return 1;
  }
  if (!(corners.quality_check_sample_count < full.quality_check_sample_count)) {
    std::cerr << "corners mode did not use fewer exact quality checks\n";
    return 1;
  }
  std::cout << "hierarchical sampling far-field quality modes passed\n";
  return 0;
}
