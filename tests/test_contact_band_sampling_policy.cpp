#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::ContactBandFarFieldMode mode =
      adasdf::ContactBandFarFieldMode::CoarseInterpolate;
  if (!adasdf::parseContactBandFarFieldMode("constant-sign", &mode) ||
      mode != adasdf::ContactBandFarFieldMode::ConstantSign) {
    std::cerr << "failed to parse constant-sign mode\n";
    return 1;
  }
  if (std::string(adasdf::toString(mode)) != "constant-sign") {
    std::cerr << "failed to stringify constant-sign mode\n";
    return 1;
  }
  adasdf::ContactBandSamplingOptions options;
  options.contact_band_width = 0.25;
  options.contact_band_layers = 2;
  options.halo_exact_layers = 1;
  const adasdf::ContactBandOptions marker = options.markerOptions();
  if (marker.contact_band_width != 0.25 ||
      marker.contact_band_layers != 2 ||
      marker.halo_exact_layers != 1) {
    std::cerr << "marker options were not propagated\n";
    return 1;
  }
  std::cout << "contact band sampling policy passed\n";
  return 0;
}
