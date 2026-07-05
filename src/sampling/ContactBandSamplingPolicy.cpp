#include "adasdf/sampling/ContactBandSamplingPolicy.h"

#include <string>

namespace adasdf {

const char* toString(ContactBandFarFieldMode mode) {
  switch (mode) {
    case ContactBandFarFieldMode::CoarseInterpolate:
      return "coarse-interpolate";
    case ContactBandFarFieldMode::ConstantSign:
      return "constant-sign";
    case ContactBandFarFieldMode::ClampedDistance:
      return "clamped-distance";
  }
  return "coarse-interpolate";
}

bool parseContactBandFarFieldMode(
    const std::string& value,
    ContactBandFarFieldMode* mode) {
  if (value == "coarse-interpolate" || value == "coarse") {
    *mode = ContactBandFarFieldMode::CoarseInterpolate;
    return true;
  }
  if (value == "constant-sign" || value == "constant") {
    *mode = ContactBandFarFieldMode::ConstantSign;
    return true;
  }
  if (value == "clamped-distance" || value == "clamped") {
    *mode = ContactBandFarFieldMode::ClampedDistance;
    return true;
  }
  return false;
}

ContactBandOptions ContactBandSamplingOptions::markerOptions() const {
  ContactBandOptions options;
  options.contact_band_width = contact_band_width;
  options.contact_band_layers = contact_band_layers;
  options.halo_exact_layers = halo_exact_layers;
  options.mark_halo_layers_exact = exact_halo_nodes;
  options.marker_mode = marker_mode;
  options.conservative_marking =
      marker_mode == ContactBandMarkerMode::ConservativeAABB;
  options.marker_safety_factor = marker_safety_factor;
  options.marker_cell_size_factor = marker_cell_size_factor;
  options.marker_min_band = marker_min_band;
  options.marker_max_band = marker_max_band;
  options.disable_global_halo = disable_global_halo;
  options.local_halo_only = local_halo_only;
  return options;
}

}  // namespace adasdf
