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
  return options;
}

}  // namespace adasdf
