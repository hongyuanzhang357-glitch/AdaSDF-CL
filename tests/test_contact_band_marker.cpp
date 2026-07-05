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
  if (!bvh.isValid()) {
    std::cerr << "failed to build triangle BVH\n";
    return 1;
  }
  adasdf::ContactBandOptions options;
  options.contact_band_width = 0.02;
  options.contact_band_layers = 1;
  options.halo_exact_layers = 1;
  const adasdf::AABB surface{{0.0, 0.0, -0.05}, {1.0, 1.0, 0.05}, true};
  const adasdf::ContactBandMask mask =
      adasdf::ContactBandMarker::markBlock(surface, 5, bvh, options);
  if (mask.contact_band_node_count == 0 || mask.exact_required_count == 0) {
    std::cerr << "surface block was not marked as contact band\n";
    return 1;
  }
  const adasdf::AABB far{{2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}, true};
  const adasdf::ContactBandMask far_mask =
      adasdf::ContactBandMarker::markBlock(far, 5, bvh, options);
  if (far_mask.contact_band_node_count != 0) {
    std::cerr << "far-field block should not contain contact-band nodes\n";
    return 1;
  }
  std::cout << "contact band marker passed\n";
  return 0;
}
