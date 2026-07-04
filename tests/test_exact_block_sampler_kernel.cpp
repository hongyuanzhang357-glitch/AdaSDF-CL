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

  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.enable_counters = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  sampler.reset(read.mesh, sampler_options, &stats);

  const adasdf::AABB bounds{{-0.25, -0.25, -0.25}, {0.75, 0.75, 0.75}, true};
  adasdf::ExactBlockSamplingKernelOptions kernel_options;
  kernel_options.signed_distance = true;
  kernel_options.enable_counters = true;
  adasdf::ExactBlockSamplingKernelStats kernel_stats;
  adasdf::AdaptiveSDFBlock kernel_block =
      adasdf::ExactBlockSamplerKernel::sample(
          bounds, 1, 2, 1, 4, sampler, kernel_options, &kernel_stats);
  adasdf::AdaptiveSDFBlock legacy_block =
      adasdf::HierarchicalBlockSampler::sampleBlockExact(
          bounds, 1, 2, 1, 4, true, true, sampler);

  if (kernel_block.phi.size() != legacy_block.phi.size() ||
      kernel_stats.sample_count != kernel_block.phi.size() ||
      kernel_stats.distance_query_count != kernel_block.phi.size() ||
      kernel_stats.sign_query_count != kernel_block.phi.size()) {
    std::cerr << "exact kernel stats mismatch\n";
    return 1;
  }
  for (std::size_t i = 0; i < kernel_block.phi.size(); ++i) {
    if (std::abs(kernel_block.phi[i] - legacy_block.phi[i]) > 1e-12) {
      std::cerr << "exact kernel output differs from legacy exact path\n";
      return 1;
    }
  }

  std::cout << "exact block sampler kernel passed\n";
  return 0;
}
