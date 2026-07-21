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
  if (!sampler.reset(read.mesh, sampler_options) || !sampler.hasBVH()) {
    std::cerr << "failed to reset BVH sampler\n";
    return 1;
  }
  adasdf::ContactBandSamplingOptions options;
  options.enable_contact_band_sampling = true;
  options.contact_band_width = 0.02;
  options.contact_band_layers = 1;
  options.halo_exact_layers = 1;
  options.far_field_resolution = 2;
  options.marker_mode = adasdf::ContactBandMarkerMode::DistanceAware;
  options.exact_contact_band_nodes = false;
  options.exact_halo_nodes = false;
  const adasdf::AABB surface{{0.25, 0.25, -0.05}, {0.75, 0.75, 0.05}, true};

  options.near_surface_exact_mode = adasdf::NearSurfaceExactMode::Off;
  const auto off = adasdf::ContactBandBlockSampler::sampleBlock(
      surface, 1, 1, 0, 6, true, sampler, sampler.bvh(), options);
  options.near_surface_exact_mode =
      adasdf::NearSurfaceExactMode::ContactBandBlocks;
  const auto blocks = adasdf::ContactBandBlockSampler::sampleBlock(
      surface, 1, 1, 0, 6, true, sampler, sampler.bvh(), options);
  if (!off.success || !blocks.success || !blocks.has_contact_band) {
    std::cerr << "exact mode sampling failed\n";
    return 1;
  }
  if (blocks.exact_node_count <= off.exact_node_count ||
      blocks.exact_node_count != blocks.block.phi.size()) {
    std::cerr << "contact-band-blocks did not increase exact nodes\n";
    return 1;
  }
  adasdf::NearSurfaceExactMode parsed = adasdf::NearSurfaceExactMode::Off;
  if (!adasdf::parseNearSurfaceExactMode("contact-band-cells", &parsed) ||
      parsed != adasdf::NearSurfaceExactMode::ContactBandCells) {
    std::cerr << "near-surface exact mode parser failed\n";
    return 1;
  }
  std::cout << "near-surface exact modes passed\n";
  return 0;
}
