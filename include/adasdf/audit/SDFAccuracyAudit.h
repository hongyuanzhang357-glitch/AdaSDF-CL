#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/audit/NearSurfaceSampleGenerator.h"
#include "adasdf/geometry/SDFModel.h"
#include "adasdf/provenance/BlockProvenance.h"

namespace adasdf {

struct SDFAccuracyAuditThresholds {
  double near_surface_p95_abs_error_limit = 5.0e-3;
  double near_surface_max_abs_error_limit = 2.0e-2;
  double p95_normal_angle_error_deg_limit = 10.0;
  bool require_zero_sign_mismatch = true;
  bool require_zero_near_surface_normal_flip = true;
};

struct SDFAccuracyAuditOptions {
  std::string case_id;
  std::filesystem::path input_sdf;
  std::filesystem::path input_stl;
  double near_band = 0.01;
  bool normal_audit = false;
  double normal_eps = 0.0;
  double sign_epsilon = 1.0e-9;
  bool cluster_sign_errors = false;
  bool exclude_zero_offset_sign = false;
  bool offset_bin_report = false;
  bool block_source_report = false;
  bool query_source_report = false;
  bool use_provenance = false;
  const BlockProvenanceSet* block_provenance = nullptr;
  std::string reference_sign_mode = "ray-majority";
  SDFAccuracyAuditThresholds thresholds;
};

struct SDFAccuracyBinStats {
  std::string key;
  std::string label;
  std::size_t sample_count = 0;
  std::size_t sign_mismatch_count = 0;
  double sign_mismatch_rate = 0.0;
  std::size_t false_inside_count = 0;
  std::size_t false_outside_count = 0;
  double p95_abs_error = 0.0;
  double p95_normal_angle_error_deg = 0.0;
};

struct SDFAccuracyAuditSample {
  std::size_t sample_id = 0;
  std::size_t surface_sample_id = 0;
  int triangle_index = -1;
  double offset = 0.0;
  Vector3 point;
  double reference_phi = 0.0;
  double sdf_phi = 0.0;
  double abs_error = 0.0;
  bool near_surface = false;
  bool out_of_domain = false;
  bool query_failed = false;
  bool sign_mismatch = false;
  bool false_inside = false;
  bool false_outside = false;
  int reference_sign = 0;
  int sdf_sign = 0;
  bool normal_checked = false;
  double normal_angle_error_deg = 0.0;
  bool normal_flip = false;
  int nearest_triangle_id = -1;
  int block_id = -1;
  int block_level = -1;
  AABB block_aabb;
  int local_i = -1;
  int local_j = -1;
  int local_k = -1;
  double local_u = 0.0;
  double local_v = 0.0;
  double local_w = 0.0;
  double corner_phi[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  std::string corner_sign_pattern;
  std::string block_source = "unknown";
  std::string query_source = "unknown";
};

struct SDFAccuracyAuditResult {
  std::string schema_id = "adasdf.sdf_accuracy_audit.v1";
  std::string case_id;
  std::filesystem::path input_stl;
  std::filesystem::path input_sdf;

  std::size_t sample_count_total = 0;
  std::size_t near_surface_sample_count = 0;
  std::size_t out_of_domain_count = 0;
  std::size_t query_failed_count = 0;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_abs_error = 0.0;
  double p50_abs_error = 0.0;
  double p95_abs_error = 0.0;
  double p99_abs_error = 0.0;

  std::size_t sign_mismatch_count = 0;
  double sign_mismatch_rate = 0.0;
  std::size_t near_surface_sign_mismatch_count = 0;
  std::size_t false_inside_count = 0;
  std::size_t false_outside_count = 0;

  bool normal_audit_enabled = false;
  std::size_t normal_check_count = 0;
  double mean_normal_angle_error_deg = 0.0;
  double p95_normal_angle_error_deg = 0.0;
  double max_normal_angle_error_deg = 0.0;
  std::size_t normal_flip_count = 0;
  std::size_t near_surface_normal_flip_count = 0;

  double sdf_query_time_ms = 0.0;
  double sdf_normal_time_ms = 0.0;
  double exact_reference_time_ms = 0.0;
  std::size_t sdf_query_count = 0;
  std::size_t exact_reference_query_count = 0;
  double ns_per_sdf_query = 0.0;
  double ns_per_reference_query = 0.0;

  double near_surface_p95_abs_error_limit = 5.0e-3;
  double near_surface_max_abs_error_limit = 2.0e-2;
  double p95_normal_angle_error_deg_limit = 10.0;
  bool near_surface_quality_passed = false;
  bool sign_quality_passed = false;
  bool normal_quality_passed = false;
  bool full_quality_passed = false;

  std::vector<SDFAccuracyBinStats> offset_bins;
  std::vector<SDFAccuracyBinStats> reference_phi_bins;
  std::vector<SDFAccuracyBinStats> block_level_bins;
  std::vector<SDFAccuracyBinStats> block_source_bins;
  std::vector<SDFAccuracyBinStats> query_source_bins;

  std::vector<SDFAccuracyAuditSample> samples;
};

class SDFAccuracyAudit {
 public:
  static SDFAccuracyAuditResult run(
      const SDFModel& sdf,
      const TriangleMesh& mesh,
      const NearSurfaceSampleSet& samples,
      const SDFAccuracyAuditOptions& options = SDFAccuracyAuditOptions{});
};

}  // namespace adasdf
