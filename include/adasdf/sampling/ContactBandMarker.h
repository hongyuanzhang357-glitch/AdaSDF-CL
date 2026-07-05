#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "adasdf/acceleration/TriangleBVH.h"
#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct ContactBandOptions {
  double contact_band_width = 1e-3;
  int contact_band_layers = 1;

  bool use_triangle_aabb_marking = true;
  bool use_cell_aabb_marking = true;

  bool mark_halo_layers_exact = true;
  int halo_exact_layers = 1;

  bool conservative_marking = true;
};

struct ContactBandMask {
  int nx = 0;
  int ny = 0;
  int nz = 0;

  std::vector<std::uint8_t> exact_required;
  std::vector<std::uint8_t> contact_band_node;
  std::vector<std::uint8_t> halo_node;

  std::size_t exact_required_count = 0;
  std::size_t contact_band_node_count = 0;
  std::size_t halo_node_count = 0;
};

class ContactBandMarker {
 public:
  static ContactBandMask markBlock(
      const AABB& block_bounds,
      int block_resolution,
      const TriangleBVH& triangle_bvh,
      const ContactBandOptions& options);
};

}  // namespace adasdf
