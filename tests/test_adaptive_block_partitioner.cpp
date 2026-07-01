#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const auto read =
        adasdf::STLReader::read((fixture_dir / "closed_cube_ascii.stl").string());
    if (!read.success) {
      std::cerr << read.error_message << "\n";
      return 1;
    }
    adasdf::AdaptiveOctreeBuildOptions octree_options;
    octree_options.max_level = 2;
    adasdf::AdaptiveOctreeBuildReport octree_report;
    const adasdf::AdaptiveOctree octree =
        adasdf::AdaptiveOctreeBuilder::build(read.mesh, octree_options, &octree_report);

    adasdf::AdaptiveBlockPartitionOptions options;
    options.block_resolution = 5;
    const adasdf::AdaptiveSDFBlockSet blocks =
        adasdf::AdaptiveBlockPartitioner::partition(octree, options);
    if (blocks.blockCount() != octree.leafCount()) {
      std::cerr << "one block per leaf was not preserved\n";
      return 1;
    }
    bool saw_near_surface = false;
    for (const adasdf::AdaptiveSDFBlock& block : blocks.blocks) {
      if (block.nx != 5 || block.ny != 5 || block.nz != 5 ||
          !block.bounds.valid || !(block.bounds.min.x < block.bounds.max.x)) {
        std::cerr << "invalid adaptive block metadata\n";
        return 1;
      }
      saw_near_surface = saw_near_surface || block.near_surface;
    }
    if (!saw_near_surface) {
      std::cerr << "near-surface block marker missing\n";
      return 1;
    }
    std::cout << "adaptive block partitioner passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_block_partitioner failed: " << exc.what()
              << "\n";
    return 1;
  }
}
