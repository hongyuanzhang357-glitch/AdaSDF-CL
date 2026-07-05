#pragma once

#include <string>

#include "adasdf/sampling/ContactBandMarker.h"

namespace adasdf {

enum class ContactFocusedSamplingMode {
  ExactFullBlock,
  ContactBandExactFarInterpolated,
  FarFieldCoarseInterpolated,
};

enum class ContactBandFarFieldMode {
  CoarseInterpolate,
  ConstantSign,
  ClampedDistance,
};

const char* toString(ContactBandFarFieldMode mode);
bool parseContactBandFarFieldMode(
    const std::string& value,
    ContactBandFarFieldMode* mode);

struct ContactBandSamplingOptions {
  bool enable_contact_band_sampling = false;

  double contact_band_width = 1e-3;
  int contact_band_layers = 1;
  int halo_exact_layers = 1;

  int far_field_resolution = 3;
  ContactBandFarFieldMode far_field_mode =
      ContactBandFarFieldMode::CoarseInterpolate;

  bool reuse_far_field_sign = true;
  bool audit_contact_band_only = true;
  bool global_quality_gate = false;

  bool exact_contact_band_nodes = true;
  bool exact_halo_nodes = true;

  bool normal_audit = false;
  double contact_band_phi_error_limit = 1e-3;
  double contact_band_normal_error_limit_deg = 5.0;

  ContactBandOptions markerOptions() const;
};

}  // namespace adasdf
