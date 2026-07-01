#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

double volume(const adasdf::AABB& box) {
  return (box.max.x - box.min.x) * (box.max.y - box.min.y) *
         (box.max.z - box.min.z);
}

}  // namespace

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const auto read =
        adasdf::STLReader::read((fixture_dir / "closed_cube_ascii.stl").string());
    if (!read.success) {
      std::cerr << read.error_message << "\n";
      return 1;
    }

    adasdf::AdaptiveOctreeBuildOptions options;
    options.max_level = 3;
    options.padding = 0.05;
    adasdf::AdaptiveOctreeBuildReport report;
    const adasdf::AdaptiveOctree octree =
        adasdf::AdaptiveOctreeBuilder::build(read.mesh, options, &report);
    if (!report.success || octree.nodeCount() <= 1 || octree.leafCount() == 0) {
      std::cerr << "adaptive octree build did not refine\n";
      return 1;
    }
    if (report.max_level_used > options.max_level ||
        report.near_surface_leaf_count == 0) {
      std::cerr << "adaptive octree stats invalid\n";
      return 1;
    }

    const adasdf::AABB root =
        octree.nodes[static_cast<std::size_t>(octree.root_id)].bounds;
    double leaf_volume = 0.0;
    for (const int leaf_id : octree.leafNodeIds()) {
      leaf_volume += volume(octree.nodes[static_cast<std::size_t>(leaf_id)].bounds);
    }
    if (std::abs(leaf_volume - volume(root)) > 1.0e-9) {
      std::cerr << "leaf bounds do not cover the root domain\n";
      return 1;
    }

    adasdf::AdaptiveOctreeBuildReport repeat_report;
    const adasdf::AdaptiveOctree repeat =
        adasdf::AdaptiveOctreeBuilder::build(read.mesh, options, &repeat_report);
    if (repeat.nodeCount() != octree.nodeCount() ||
        repeat.leafNodeIds() != octree.leafNodeIds()) {
      std::cerr << "adaptive octree build is not deterministic\n";
      return 1;
    }

    std::cout << "adaptive octree builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_octree_builder failed: " << exc.what() << "\n";
    return 1;
  }
}
