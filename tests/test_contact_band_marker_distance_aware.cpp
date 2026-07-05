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
  adasdf::TriangleBVHBuildReport report;
  const adasdf::TriangleBVH bvh =
      adasdf::TriangleBVHBuilder::build(read.mesh, {}, &report);
  const adasdf::AABB surface{{0.0, 0.0, -0.05}, {1.0, 1.0, 0.05}, true};

  adasdf::ContactBandOptions conservative;
  conservative.contact_band_width = 0.02;
  conservative.contact_band_layers = 1;
  conservative.halo_exact_layers = 1;
  conservative.marker_mode = adasdf::ContactBandMarkerMode::ConservativeAABB;
  const adasdf::ContactBandMask conservative_mask =
      adasdf::ContactBandMarker::markBlock(surface, 6, bvh, conservative);

  adasdf::ContactBandOptions distance = conservative;
  distance.marker_mode = adasdf::ContactBandMarkerMode::DistanceAware;
  distance.marker_cell_size_factor = 0.5;
  distance.local_halo_only = true;
  distance.disable_global_halo = true;
  const adasdf::ContactBandMask distance_mask =
      adasdf::ContactBandMarker::markBlock(surface, 6, bvh, distance);

  if (distance_mask.contact_band_node_count == 0 ||
      distance_mask.exact_required_count == 0) {
    std::cerr << "distance-aware marker missed cube surface cells\n";
    return 1;
  }
  if (distance_mask.contact_band_node_count >
      conservative_mask.contact_band_node_count) {
    std::cerr << "distance-aware marker should not mark more nodes here\n";
    return 1;
  }
  if (distance_mask.distance_refined_cell_count == 0) {
    std::cerr << "distance-aware marker did not run distance refinement\n";
    return 1;
  }
  std::cout << "distance-aware contact-band marker passed\n";
  return 0;
}
