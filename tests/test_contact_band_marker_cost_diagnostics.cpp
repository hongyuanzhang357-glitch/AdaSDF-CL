#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path cube =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const auto read = adasdf::STLReader::read(cube.string());
    if (!read.success) {
      std::cerr << "failed to read cube fixture\n";
      return 1;
    }

    adasdf::TriangleBVHBuildReport report;
    const adasdf::TriangleBVH bvh =
        adasdf::TriangleBVHBuilder::build(read.mesh, {}, &report);
    if (!report.success || !bvh.isValid()) {
      std::cerr << "failed to build cube BVH\n";
      return 1;
    }
    adasdf::AABB block;
    block.valid = true;
    block.min = {-0.05, -0.05, -0.05};
    block.max = {0.25, 0.25, 0.25};

    adasdf::ContactBandOptions options;
    options.marker_mode = adasdf::ContactBandMarkerMode::DistanceAware;
    options.contact_band_width = 5e-4;
    options.local_halo_only = true;
    options.marker_cell_size_factor = 0.5;
    options.marker_safety_factor = 1.0;

    const adasdf::ContactBandMask mask =
        adasdf::ContactBandMarker::markBlock(block, 8, bvh, options);
    if (mask.candidate_cell_count == 0 || mask.candidate_triangle_count == 0 ||
        mask.refined_candidate_count == 0 ||
        mask.accepted_contact_cell_count == 0 ||
        mask.marked_node_count == 0) {
      std::cerr << "marker cost diagnostics did not count candidates\n";
      return 1;
    }
    if (!std::isfinite(mask.marker_time_ms) ||
        !std::isfinite(mask.triangle_bvh_query_time_ms) ||
        !std::isfinite(mask.box_triangle_distance_time_ms) ||
        mask.marker_time_ms < 0.0 ||
        mask.triangle_bvh_query_time_ms < 0.0 ||
        mask.box_triangle_distance_time_ms < 0.0) {
      std::cerr << "marker cost diagnostics are not finite\n";
      return 1;
    }

    std::cout << "contact-band marker cost diagnostics passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_band_marker_cost_diagnostics failed: "
              << exc.what() << "\n";
    return 1;
  }
}
