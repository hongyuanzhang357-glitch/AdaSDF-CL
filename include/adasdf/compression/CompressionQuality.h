#pragma once

#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

struct CompressionQualityOptions {
  int samples_per_block_axis = 4;
  double sign_epsilon = 1.0e-12;
  double near_surface_band = 1.0e-2;
};

struct CompressionQualityReport {
  bool success = false;
  std::string error_message;

  std::size_t sample_count = 0;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_abs_error = 0.0;

  std::size_t sign_mismatch_count = 0;
  std::size_t near_surface_sign_mismatch_count = 0;

  double compression_ratio = 1.0;

  std::vector<std::string> warnings;
};

class CompressionQuality {
 public:
  static CompressionQualityReport compare(
      const SDFModel& reference_dense_model,
      const SDFModel& compressed_model,
      const CompressionQualityOptions& options = CompressionQualityOptions{});
};

}  // namespace adasdf
