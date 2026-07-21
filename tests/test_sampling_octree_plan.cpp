#include <adasdf/adasdf.h>

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
    adasdf::NarrowBandBrickBuildOptions options;
    options.max_sampling_level = 3;
    options.sampling_contact_band_width = 0.08;
    std::string error;
    if (!adasdf::parseBrickLevelMap(
            "0-2:0,3-5:1",
            &options.brick_level_map,
            &error)) {
      std::cerr << error << "\n";
      return 1;
    }
    const adasdf::SamplingOctreePlan sampling =
        adasdf::SamplingOctree::build(read.mesh, sampler, options);
    const adasdf::CompressionBrickTreePlan bricks =
        adasdf::CompressionBrickTree::build(sampling, options);
    if (sampling.nodes.empty() || bricks.bricks.empty() ||
        sampling.nodes.size() == bricks.bricks.size()) {
      std::cerr << "sampling tree and brick tree were not independently planned\n";
      return 1;
    }
    if (sampling.contact_band_node_count_by_level.empty() ||
        sampling.node_count_by_level.empty()) {
      std::cerr << "sampling tree missing per-level stats\n";
      return 1;
    }
    bool saw_parent = false;
    bool saw_interpolatable = false;
    bool saw_exact_demand = false;
    for (const adasdf::SamplingOctreeNode& node : sampling.nodes) {
      saw_parent = saw_parent || node.parent_id != -1;
      saw_interpolatable = saw_interpolatable || node.can_be_interpolated;
      saw_exact_demand = saw_exact_demand || node.exact_sample_required;
    }
    if (!saw_parent || !saw_interpolatable || !saw_exact_demand) {
      std::cerr << "sampling tree missing node demand metadata\n";
      return 1;
    }
    std::cout << "sampling octree plan passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_sampling_octree_plan failed: " << exc.what() << "\n";
    return 1;
  }
}
