#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/audit/CellZeroCrossingDiagnostic.h"
#include "adasdf/audit/LocalExactSubcellProbe.h"
#include "adasdf/audit/SDFAccuracyAudit.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/provenance/BlockProvenance.h"

namespace adasdf {

struct MismatchCellForensicsOptions {
  std::string case_id;
  double near_band = 0.01;
  double sign_epsilon = 1.0e-9;
  std::vector<int> local_subgrids = {2, 4};
  std::size_t max_cells = 10000;
  bool enable_27_point_stencil = true;
  const BlockProvenanceSet* provenance = nullptr;
};

struct MismatchCellPatternStats {
  std::string key;
  std::size_t sample_count = 0;
  std::size_t sign_mismatch_count = 0;
  std::size_t false_inside_count = 0;
  std::size_t false_outside_count = 0;
  double p95_abs_error = 0.0;
  double p95_normal_angle_error_deg = 0.0;
  double average_block_level = 0.0;
  std::vector<int> top_block_ids;
};

struct MismatchCellSubgridSummary {
  int subgrid = 0;
  std::size_t fixed_count = 0;
  std::size_t remaining_mismatch_count = 0;
};

struct MismatchCellSampleDiagnostic {
  std::size_t sample_id = 0;
  Vector3 point;
  double reference_phi = 0.0;
  double sdf_phi = 0.0;
  int reference_sign = 0;
  int sdf_sign = 0;
  bool false_inside = false;
  bool false_outside = false;
  double abs_error = 0.0;

  int nearest_triangle_id = -1;
  Vector3 nearest_point_on_triangle;
  Vector3 nearest_triangle_normal;

  int block_id = -1;
  int block_level = -1;
  AABB block_aabb;
  int block_nx = 0;
  int block_ny = 0;
  int block_nz = 0;
  int local_i = -1;
  int local_j = -1;
  int local_k = -1;
  double local_u = 0.0;
  double local_v = 0.0;
  double local_w = 0.0;
  double distance_to_cell_boundary = 0.0;
  bool block_boundary_cell = false;
  bool mixed_level_boundary_cell = false;

  bool contact_band_block = false;
  bool coverage_promoted_block = false;
  bool far_field_block = false;
  bool provenance_exists = false;

  AABB cell_aabb;
  double corner_phi[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double exact_corner_phi[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double exact_center_phi = 0.0;
  std::vector<double> exact_stencil_phi;
  std::string corner_sign_pattern;
  std::string corner_pattern_category;
  std::size_t positive_corner_count = 0;
  std::size_t negative_corner_count = 0;
  std::size_t near_zero_corner_count = 0;
  double trilinear_phi = 0.0;
  double exact_bvh_phi = 0.0;

  bool triangle_cell_aabb_overlap = false;
  bool expanded_triangle_cell_aabb_overlap = false;
  double box_triangle_approx_distance = 0.0;
  bool nearest_surface_point_inside_cell = false;
  bool likely_zero_crossing_inside_cell = false;
  std::string zero_crossing_category;
  bool reference_sign_suspicious = false;
  bool block_lookup_suspicious = false;

  std::map<int, LocalExactSubcellProbeResult> local_subgrid_results;
};

struct MismatchCellForensicsResult {
  std::string schema_id = "adasdf.mismatch_cell_forensics.v1";
  std::string case_id;
  std::size_t sample_count = 0;
  std::size_t sign_mismatch_count = 0;
  std::size_t false_inside_count = 0;
  std::size_t false_outside_count = 0;
  double p95_abs_error = 0.0;

  std::size_t corner_pattern_all_positive_count = 0;
  std::size_t corner_pattern_all_negative_count = 0;
  std::size_t corner_pattern_mixed_sign_count = 0;
  std::size_t corner_pattern_has_near_zero_corner_count = 0;
  std::size_t corner_pattern_checkerboard_like_count = 0;
  std::size_t corner_pattern_single_opposite_corner_count = 0;
  std::size_t corner_pattern_edge_crossing_count = 0;
  std::size_t corner_pattern_face_crossing_count = 0;

  std::size_t surface_crossing_with_same_sign_corners_count = 0;
  std::size_t surface_crossing_with_mixed_corners_count = 0;
  std::size_t no_surface_crossing_but_sign_mismatch_count = 0;
  std::size_t block_lookup_suspicious_count = 0;
  std::size_t reference_sign_suspicious_count = 0;
  std::size_t block_boundary_mismatch_count = 0;
  std::size_t mixed_level_boundary_mismatch_count = 0;

  std::vector<MismatchCellPatternStats> corner_pattern_stats;
  std::vector<MismatchCellPatternStats> zero_crossing_stats;
  std::vector<MismatchCellSubgridSummary> subgrid_summaries;
  std::string false_inside_major_corner_pattern = "none";
  std::string false_outside_major_corner_pattern = "none";

  bool cell_resolution_insufficient_likely = false;
  std::vector<MismatchCellSampleDiagnostic> samples;
};

class MismatchCellForensics {
 public:
  static MismatchCellForensicsResult run(
      const AdaptiveBlockSDFModel& model,
      const TriangleMesh& mesh,
      const std::vector<SDFAccuracyAuditSample>& mismatch_samples,
      const MismatchCellForensicsOptions& options =
          MismatchCellForensicsOptions{});
};

}  // namespace adasdf
