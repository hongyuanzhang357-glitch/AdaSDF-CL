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
    std::cerr << "failed to reset sampler\n";
    return 1;
  }
  adasdf::ContactBandSamplingOptions options;
  options.enable_contact_band_sampling = true;
  options.contact_band_width = 0.02;
  options.contact_band_layers = 1;
  options.halo_exact_layers = 1;
  options.far_field_resolution = 2;
  const adasdf::AABB surface{{0.0, 0.0, -0.05}, {1.0, 1.0, 0.05}, true};
  const auto surface_result = adasdf::ContactBandBlockSampler::sampleBlock(
      surface,
      1,
      1,
      0,
      5,
      true,
      sampler,
      sampler.bvh(),
      options);
  if (!surface_result.success || !surface_result.has_contact_band ||
      surface_result.exact_node_count < surface_result.mask.contact_band_node_count) {
    std::cerr << "contact-band surface block sampling failed\n";
    return 1;
  }
  const adasdf::AABB far{{2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}, true};
  const auto far_result = adasdf::ContactBandBlockSampler::sampleBlock(
      far,
      2,
      2,
      0,
      5,
      true,
      sampler,
      sampler.bvh(),
      options);
  if (!far_result.success || far_result.has_contact_band ||
      far_result.exact_node_count >= far_result.block.phi.size() ||
      far_result.predicted_node_count != far_result.block.phi.size()) {
    std::cerr << "far-field block should be coarse/interpolated\n";
    return 1;
  }
  std::cout << "contact band block sampler passed\n";
  return 0;
}
