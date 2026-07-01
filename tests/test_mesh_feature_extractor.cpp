#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto closed = fixture_dir / "closed_cube_ascii.stl";
    const auto open = fixture_dir / "open_cube_missing_face_ascii.stl";

    const adasdf::MeshFeatureSummary closed_features =
        adasdf::MeshFeatureExtractor::fromSTL(closed.string());
    if (!closed_features.valid || !closed_features.watertight ||
        closed_features.triangle_count == 0 ||
        !(closed_features.aabb_diagonal > 0.0) ||
        closed_features.readiness_score <= 0) {
      std::cerr << "closed cube feature extraction failed\n";
      return 1;
    }

    const adasdf::MeshFeatureSummary open_features =
        adasdf::MeshFeatureExtractor::fromSTL(open.string());
    if (!open_features.valid || open_features.watertight ||
        open_features.boundary_edge_count == 0) {
      std::cerr << "open cube feature extraction failed\n";
      return 1;
    }

    const adasdf::MeshFeatureSummary missing =
        adasdf::MeshFeatureExtractor::fromSTL(
            (fixture_dir / "missing_input.stl").string());
    if (missing.valid || missing.warnings.empty()) {
      std::cerr << "missing STL did not produce invalid feature summary\n";
      return 1;
    }
    std::cout << "mesh feature extractor passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_feature_extractor failed: " << exc.what() << "\n";
    return 1;
  }
}
