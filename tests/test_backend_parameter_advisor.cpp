#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  const auto read = adasdf::STLReader::read(
      (std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
       "closed_cube_ascii.stl").string());
  if (!read.success) {
    std::cerr << "failed to load advisor fixture\n";
    return 1;
  }
  const auto diagnostics = adasdf::MeshDiagnostics::analyze(read.mesh);
  const auto features =
      adasdf::GeometryFeatureExtractor::fromMesh(read.mesh, diagnostics);
  if (!features.valid || features.mesh_hash.empty() ||
      features.surface_area <= 0.0 || features.edge_length_mean <= 0.0 ||
      !features.watertight) {
    std::cerr << "versioned geometry feature extraction failed\n";
    return 1;
  }

  adasdf::SDFCreationConstraints constraints;
  constraints.max_sdf_file_bytes = 16 * 1024 * 1024;
  constraints.max_decoded_block_bytes = 256 * 1024;
  constraints.max_zero_surface_abs_error = 0.01;
  adasdf::HeuristicBackendParameterAdvisor heuristic;
  const auto first = heuristic.advise(features, constraints);
  const auto repeat = heuristic.advise(features, constraints);
  if (!first.success || !repeat.success || first.used_fallback ||
      first.source != "heuristic-v1" ||
      first.parameters.octree_max_depth !=
          repeat.parameters.octree_max_depth ||
      first.parameters.block_target_nodes <= 0 ||
      first.parameters.block_max_tensor_dimension <
          first.parameters.block_min_tensor_dimension ||
      !(first.parameters.octree_interpolation_residual_scale > 0.0) ||
      first.parameters.compression_method !=
          repeat.parameters.compression_method) {
    std::cerr << "heuristic advisor is not deterministic\n";
    return 1;
  }

  adasdf::SDFCreationConstraints invalid = constraints;
  invalid.max_zero_surface_abs_error = 0.0;
  const auto rejected = heuristic.advise(features, invalid);
  if (rejected.success || rejected.warnings.empty()) {
    std::cerr << "heuristic advisor accepted invalid constraints\n";
    return 1;
  }
  return 0;
}
