#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"

namespace adasdf {

struct SpeedQualityBenchmarkOptions {
  AdaptiveBlockSDFBuildOptions build_options;
  int comparison_samples_per_axis = 5;
};

struct SpeedQualityBenchmarkResult {
  bool success = false;
  std::string error_message;

  double exact_build_time_ms = 0.0;
  double hierarchical_build_time_ms = 0.0;
  double speedup = 0.0;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_error = 0.0;

  std::size_t sign_mismatch_count = 0;
  std::size_t near_surface_sign_mismatch_count = 0;

  std::size_t exact_sample_count = 0;
  std::size_t hierarchical_exact_sample_count = 0;
  std::size_t predicted_sample_count = 0;
  std::size_t fallback_block_count = 0;

  bool quality_gate_passed = false;

  std::vector<std::string> warnings;
};

class SpeedQualityBenchmark {
 public:
  static SpeedQualityBenchmarkResult run(
      const TriangleMesh& mesh,
      const SpeedQualityBenchmarkOptions& options);

  static SpeedQualityBenchmarkResult runSTL(
      const std::string& stl_path,
      const SpeedQualityBenchmarkOptions& options);
};

}  // namespace adasdf
