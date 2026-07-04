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
  std::string case_id;

  int max_level = 0;
  int block_resolution = 0;
  int coarse_resolution = 0;
  int quality_check_samples = 0;
  int transition_quality_check_samples = 0;
  FarFieldQualityCheckMode far_field_quality_check =
      FarFieldQualityCheckMode::Corners;
  double far_field_safety_factor = 0.0;
  FarFieldSignPolicy far_field_sign_policy = FarFieldSignPolicy::Exact;
  NearSurfaceSamplingMode near_surface_mode = NearSurfaceSamplingMode::Exact;
  double near_surface_band_factor = 0.0;
  int near_surface_check_samples = 0;
  int halo_exact_layers = 0;

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
  std::size_t local_fallback_node_count = 0;

  bool quality_gate_passed = false;
  bool effective_speedup_claim_allowed = false;
  HierarchicalSamplingDiagnostics diagnostics;

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
