#pragma once

#include <string>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/query/ExpandedSDF.h"

namespace adasdf {

enum class SDFSignClass {
  Inside,
  Outside,
  Ambiguous
};

struct ExpansionQualityOptions {
  int num_samples = 10000;
  double near_surface_band = 1e-3;
  double sign_epsilon = 1e-9;
  unsigned int seed = 12345;

  bool include_near_surface_samples = true;
  bool include_volume_samples = true;
};

struct ExpansionQualityReport {
  int num_samples = 0;
  int num_finite_samples = 0;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_abs_error = 0.0;

  int sign_mismatch_count = 0;
  double sign_mismatch_rate = 0.0;

  int ambiguous_sign_count = 0;
  double ambiguous_sign_rate = 0.0;

  int near_surface_sample_count = 0;
  int near_surface_sign_mismatch_count = 0;
  double near_surface_sign_mismatch_rate = 0.0;

  int fallback_count = 0;
  double fallback_rate = 0.0;

  int worst_point_id = -1;
  Vector3 worst_point;
  double worst_direct_phi = 0.0;
  double worst_expanded_phi = 0.0;

  std::string direct_backend;
  std::string expanded_backend;
};

class ExpansionQuality {
 public:
  static ExpansionQualityReport compareAgainstDirect(
      const SDFModel& direct_model,
      const ExpandedSDF& expanded,
      const ExpansionQualityOptions& options);
};

SDFSignClass classifySDFSign(double phi, double sign_epsilon);
bool isStrictSignMismatch(
    double direct_phi,
    double expanded_phi,
    double sign_epsilon);
const char* toString(SDFSignClass sign_class);

}  // namespace adasdf
