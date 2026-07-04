#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  adasdf::FarFieldSignPolicy policy = adasdf::FarFieldSignPolicy::Exact;
  if (!adasdf::parseFarFieldSignPolicy("reuse-coarse", &policy) ||
      policy != adasdf::FarFieldSignPolicy::ReuseCoarse ||
      std::string(adasdf::toString(policy)) != "reuse-coarse" ||
      !adasdf::parseFarFieldSignPolicy("constant", &policy) ||
      policy != adasdf::FarFieldSignPolicy::Constant) {
    std::cerr << "far-field sign policy parse failed\n";
    return 1;
  }

  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  sampler.reset(read.mesh, sampler_options, &stats);

  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.far_field_sign_policy = adasdf::FarFieldSignPolicy::Constant;
  options.near_surface_mode = adasdf::NearSurfaceSamplingMode::Exact;
  const adasdf::AABB near_bounds{{-0.1, -0.1, -0.1}, {1.1, 1.1, 1.1}, true};
  const auto near = adasdf::HierarchicalBlockSampler::sampleBlock(
      near_bounds, 10, 10, 1, 4, true, true, sampler, options);
  if (!near.success || near.coarse_sample_count != 0 ||
      near.diagnostics.near_surface_banded_block_count != 0) {
    std::cerr << "far-field sign policy affected near-surface exact path\n";
    return 1;
  }
  std::cout << "far-field sign policy passed\n";
  return 0;
}
