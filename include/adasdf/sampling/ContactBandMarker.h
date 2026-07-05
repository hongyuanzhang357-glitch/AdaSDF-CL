#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "adasdf/acceleration/TriangleBVH.h"
#include "adasdf/geometry/Transform.h"

namespace adasdf {

enum class ContactBandMarkerMode {
  ConservativeAABB,
  DistanceAware,
  Hybrid,
};

const char* toString(ContactBandMarkerMode mode);
bool parseContactBandMarkerMode(
    const std::string& value,
    ContactBandMarkerMode* mode);

struct ContactBandOptions {
  double contact_band_width = 1e-3;
  int contact_band_layers = 1;

  bool use_triangle_aabb_marking = true;
  bool use_cell_aabb_marking = true;

  bool mark_halo_layers_exact = true;
  int halo_exact_layers = 1;

  bool conservative_marking = true;
  ContactBandMarkerMode marker_mode = ContactBandMarkerMode::ConservativeAABB;
  double marker_safety_factor = 1.0;
  double marker_cell_size_factor = 0.75;
  double marker_min_band = 0.0;
  double marker_max_band = 0.0;
  bool disable_global_halo = false;
  bool local_halo_only = false;
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

  std::string marker_mode = "conservative-aabb";
  std::size_t candidate_triangle_aabb_overlap_count = 0;
  std::size_t distance_refined_cell_count = 0;
  std::size_t distance_rejected_cell_count = 0;
  std::size_t marked_cell_count = 0;
  std::size_t marked_node_count = 0;
  std::size_t local_halo_node_count = 0;
  std::size_t global_halo_node_count = 0;
  double overmark_ratio_estimate = 0.0;
  double marker_time_ms = 0.0;
  double distance_refinement_time_ms = 0.0;
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
