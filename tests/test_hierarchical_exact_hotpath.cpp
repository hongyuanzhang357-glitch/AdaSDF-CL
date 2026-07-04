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
  sampler_options.enable_counters = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  sampler.reset(read.mesh, sampler_options, &stats);

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.hierarchical_diagnostics = true;
  options.near_surface_mode = adasdf::NearSurfaceSamplingMode::Exact;
  const adasdf::AABB bounds{{-0.1, -0.1, -0.1}, {1.1, 1.1, 1.1}, true};
  const auto sampled = adasdf::HierarchicalBlockSampler::sampleBlock(
      bounds, 8, 8, 1, 4, true, true, sampler, options);
  if (!sampled.success || sampled.coarse_sample_count != 0 ||
      sampled.exact_sample_count != 64 ||
      sampled.diagnostics.exact_bvh_sample_count != 64) {
    std::cerr << "near-surface exact hot path did not stay short\n";
    return 1;
  }
  std::cout << "hierarchical exact hotpath passed\n";
  return 0;
}
