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

  adasdf::NarrowBandBrickBuildOptions base;
  std::string error;
  adasdf::parseBrickLevelMap("0-3:0", &base.brick_level_map, &error);
  base.max_sampling_level = 3;
  base.brick_min_tensor_dim = 8;
  base.brick_target_tensor_dim = 8;
  base.brick_max_tensor_dim = 8;
  base.sampling_contact_band_width = 0.01;

  adasdf::NarrowBandBrickBuildStats before;
  adasdf::NarrowBandBrickSDFBuilder::fromMesh(read.mesh, base, &before);

  adasdf::NarrowBandBrickBuildOptions enriched = base;
  enriched.contact_exact_min_node_ratio = 0.25;
  enriched.contact_exact_from_zero_crossing_cells = true;
  enriched.contact_exact_stencil = 2;
  adasdf::NarrowBandBrickBuildStats after;
  adasdf::NarrowBandBrickSDFBuilder::fromMesh(read.mesh, enriched, &after);

  const std::size_t target = static_cast<std::size_t>(
      0.25 * static_cast<double>(after.total_tensor_nodes));
  if (after.total_exact_source_nodes <= before.total_exact_source_nodes ||
      after.total_exact_source_nodes < target) {
    std::cerr << "contact exact enrichment did not increase exact nodes\n";
    return 1;
  }
  std::cout << "contact exact enrichment passed\n";
  return 0;
}

