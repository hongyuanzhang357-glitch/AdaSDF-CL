#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    if (!read.success) {
      std::cerr << "failed to read cube fixture\n";
      return 1;
    }
    adasdf::BVHSDFSampler sampler;
    adasdf::BVHSDFSamplerOptions sampler_options;
    sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    sampler_options.signed_distance = true;
    if (!sampler.reset(read.mesh, sampler_options)) {
      std::cerr << "failed to build sampler\n";
      return 1;
    }
    adasdf::CompressionBrick brick;
    brick.brick_id = 3;
    brick.brick_level = 1;
    brick.bounds.valid = true;
    brick.bounds.min = {-0.1, -0.1, -0.1};
    brick.bounds.max = {1.1, 1.1, 1.1};
    brick.tensor_nx = 6;
    brick.tensor_ny = 6;
    brick.tensor_nz = 6;
    brick.contact_band_node_count = 1;

    adasdf::NarrowBandBrickBuildOptions options;
    options.sampling_contact_band_width = 0.2;
    options.tensor_fill = adasdf::NarrowBandTensorFillMode::ContactExactFarInterp;
    const adasdf::BrickTensorResult result =
        adasdf::BrickTensorGenerator::generate(brick, sampler, options);
    if (result.block.block_id != 3 ||
        result.block.nx != 6 ||
        result.block.phi.size() != 216 ||
        !std::all_of(result.block.phi.begin(), result.block.phi.end(), [](double v) {
          return std::isfinite(v);
        }) ||
        result.brick.exact_source_node_count == 0 ||
        result.brick.interpolated_fill_node_count == 0) {
      std::cerr << "brick tensor generation failed\n";
      return 1;
    }
    std::cout << "brick tensor generator passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_brick_tensor_generator failed: " << exc.what() << "\n";
    return 1;
  }
}
