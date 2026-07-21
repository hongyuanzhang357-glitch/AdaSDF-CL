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
  adasdf::NarrowBandBrickBuildOptions options;
  std::string error;
  adasdf::parseBrickLevelMap("0-3:0", &options.brick_level_map, &error);
  options.max_sampling_level = 3;
  options.brick_min_tensor_dim = 8;
  options.brick_target_tensor_dim = 8;
  options.brick_max_tensor_dim = 8;
  options.sampling_contact_band_width = 0.10;
  options.sign_protected_fill = true;
  options.fill_sign_check = true;
  options.fill_sign_fallback = "exact";
  options.contact_exact_from_zero_crossing_cells = true;
  options.contact_exact_stencil = 2;

  adasdf::NarrowBandBrickBuildStats stats;
  auto model =
      adasdf::NarrowBandBrickSDFBuilder::fromMesh(read.mesh, options, &stats);
  if (!model || !model->isValid() || !stats.sign_protected_fill_enabled ||
      stats.sign_check_node_count == 0 ||
      stats.fill_fallback_exact_node_count == 0) {
    std::cerr << "sign-protected tensor fill did not trigger\n";
    return 1;
  }
  std::cout << "sign-protected tensor fill passed\n";
  return 0;
}

