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
    const auto read = adasdf::STLReader::read(fixture.string());
    if (!read.success) {
      std::cerr << "failed to read cube fixture\n";
      return 1;
    }

    adasdf::AdaptiveOctreeBuildOptions mixed_options;
    mixed_options.min_level = 1;
    mixed_options.max_level = 3;
    mixed_options.leaf_mode = adasdf::AdaptiveLeafMode::Mixed;
    mixed_options.target_near_surface_error = 1e-3;
    adasdf::AdaptiveOctree mixed =
        adasdf::AdaptiveOctreeBuilder::build(read.mesh, mixed_options);
    adasdf::AdaptiveBlockPartitionOptions partition_options;
    partition_options.block_resolution = 4;
    const auto mixed_blocks =
        adasdf::AdaptiveBlockPartitioner::partition(mixed, partition_options);
    const auto mixed_stats = adasdf::computeAdaptiveTreeStats(
        mixed,
        mixed_blocks,
        partition_options.block_resolution,
        mixed_options.max_level);
    if (!mixed_stats.mixed_level_present ||
        mixed_stats.appears_uniform_max_level ||
        mixed_stats.leaf_block_count >=
            mixed_stats.uniform_max_level_leaf_count) {
      std::cerr << "mixed leaf mode did not preserve mixed levels\n";
      return 1;
    }

    adasdf::AdaptiveOctreeBuildOptions uniform_options = mixed_options;
    uniform_options.leaf_mode = adasdf::AdaptiveLeafMode::Uniform;
    adasdf::AdaptiveOctree uniform =
        adasdf::AdaptiveOctreeBuilder::build(read.mesh, uniform_options);
    const auto uniform_blocks =
        adasdf::AdaptiveBlockPartitioner::partition(uniform, partition_options);
    const auto uniform_stats = adasdf::computeAdaptiveTreeStats(
        uniform,
        uniform_blocks,
        partition_options.block_resolution,
        uniform_options.max_level);
    if (!uniform_stats.appears_uniform_max_level ||
        uniform_stats.mixed_level_present ||
        uniform_stats.leaf_block_count !=
            uniform_stats.uniform_max_level_leaf_count) {
      std::cerr << "uniform leaf mode did not keep max-level behavior\n";
      return 1;
    }
    std::cout << "mixed level leaf builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mixed_level_leaf_builder failed: " << exc.what()
              << "\n";
    return 1;
  }
}
