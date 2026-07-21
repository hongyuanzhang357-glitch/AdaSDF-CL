#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "adasdf/audit/NearSurfaceSampleGenerator.h"
#include "adasdf/geometry/SDFModel.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/provenance/BlockProvenance.h"

namespace adasdf {

struct CoverageAuditOptions {
  double near_band = 0.01;
  int surface_samples = 3000;
  std::vector<double> offsets = {-0.004, -0.002, -0.001,
                                 0.001, 0.002, 0.004};

  bool include_negative_offsets = true;
  bool include_positive_offsets = true;

  bool require_contact_block_hit = true;
  bool require_exact_cell_hit = false;

  bool cluster_by_block = true;
  bool cluster_by_level = true;
};

struct CoverageMissRecord {
  Vector3 point;
  double reference_phi = 0.0;
  int reference_sign = 0;

  int block_id = -1;
  int block_level = -1;

  bool hit_contact_block = false;
  bool hit_far_field_block = false;
  bool hit_predicted_region = false;
  bool hit_mixed_level_boundary = false;

  int nearest_triangle_id = -1;
};

struct CoverageAuditResult {
  std::size_t total_samples = 0;
  std::size_t near_surface_samples = 0;
  std::size_t covered_samples = 0;
  std::size_t missed_samples = 0;

  double missed_rate = 0.0;

  std::map<int, std::size_t> missed_by_level;
  std::map<int, std::size_t> missed_by_block_id;

  std::vector<int> top_missed_block_ids;

  bool coverage_passed = false;

  std::vector<CoverageMissRecord> representative_misses;
};

class ContactBandCoverageAudit {
 public:
  static CoverageAuditResult run(
      const SDFModel& sdf,
      const TriangleMesh& mesh,
      const CoverageAuditOptions& options,
      const BlockProvenanceSet* provenance = nullptr);

  static CoverageAuditResult run(
      const SDFModel& sdf,
      const TriangleMesh& mesh,
      const NearSurfaceSampleSet& samples,
      const CoverageAuditOptions& options,
      const BlockProvenanceSet* provenance = nullptr);
};

}  // namespace adasdf
