#pragma once

#include <cstddef>

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/sampling/ContactBandMarker.h"
#include "adasdf/sampling/ContactBandSamplingPolicy.h"

namespace adasdf {

struct ContactBandQualityMetrics {
  bool contact_band_quality_passed = false;
  bool coverage_passed = true;

  std::size_t contact_band_check_count = 0;
  std::size_t contact_band_coverage_check_count = 0;
  std::size_t missed_contact_band_point_count = 0;
  std::size_t missed_contact_band_cell_count = 0;

  double contact_band_max_abs_error = 0.0;
  double contact_band_mean_abs_error = 0.0;
  double contact_band_rms_error = 0.0;
  double contact_band_p95_error = 0.0;

  std::size_t contact_band_sign_mismatch_count = 0;
  std::size_t near_surface_sign_mismatch_count = 0;

  double mean_normal_angle_error_deg = 0.0;
  double p95_normal_angle_error_deg = 0.0;
  double max_normal_angle_error_deg = 0.0;

  std::size_t normal_flip_count = 0;
  std::size_t near_surface_normal_flip_count = 0;
};

class ContactBandQualityAudit {
 public:
  static ContactBandQualityMetrics auditBlock(
      const AdaptiveSDFBlock& candidate,
      const AdaptiveSDFBlock& exact_reference,
      const ContactBandMask& mask,
      const ContactBandSamplingOptions& options);

  static ContactBandQualityMetrics merge(
      const ContactBandQualityMetrics& lhs,
      const ContactBandQualityMetrics& rhs);

  static void finalize(
      ContactBandQualityMetrics* metrics,
      const ContactBandSamplingOptions& options);
};

}  // namespace adasdf
