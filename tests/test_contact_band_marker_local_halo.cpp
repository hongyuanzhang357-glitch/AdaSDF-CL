#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::TriangleMesh makeSmallTriangleMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {
      {0.45, 0.45, 0.50},
      {0.55, 0.45, 0.50},
      {0.50, 0.55, 0.50},
  };
  mesh.triangles = {{0, 1, 2, 0}};
  return mesh;
}

}  // namespace

int main() {
  const adasdf::TriangleMesh mesh = makeSmallTriangleMesh();
  const adasdf::TriangleBVH bvh =
      adasdf::TriangleBVHBuilder::build(mesh, {}, nullptr);
  const adasdf::AABB block{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};

  adasdf::ContactBandOptions global_halo;
  global_halo.contact_band_width = 0.001;
  global_halo.contact_band_layers = 1;
  global_halo.halo_exact_layers = 1;
  global_halo.marker_mode = adasdf::ContactBandMarkerMode::DistanceAware;
  global_halo.marker_cell_size_factor = 0.25;
  const adasdf::ContactBandMask global_mask =
      adasdf::ContactBandMarker::markBlock(block, 7, bvh, global_halo);

  adasdf::ContactBandOptions local_halo = global_halo;
  local_halo.local_halo_only = true;
  local_halo.disable_global_halo = true;
  const adasdf::ContactBandMask local_mask =
      adasdf::ContactBandMarker::markBlock(block, 7, bvh, local_halo);

  if (global_mask.contact_band_node_count == 0 ||
      local_mask.contact_band_node_count == 0) {
    std::cerr << "small triangle was not marked as contact band\n";
    return 1;
  }
  if (global_mask.global_halo_node_count == 0) {
    std::cerr << "global halo mode should mark block-boundary halo nodes\n";
    return 1;
  }
  if (local_mask.global_halo_node_count != 0 ||
      local_mask.local_halo_node_count == 0) {
    std::cerr << "local halo mode should avoid global halo and mark local halo\n";
    return 1;
  }
  if (!(local_mask.exact_required_count < global_mask.exact_required_count)) {
    std::cerr << "local halo should reduce exact nodes for a small contact patch\n";
    return 1;
  }
  std::cout << "contact-band local halo marker passed\n";
  return 0;
}
