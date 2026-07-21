#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/coverage/ContactBandCoverageAudit.h"
#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/provenance/BlockProvenance.h"

namespace adasdf {

enum class CoveragePromotionMode {
  MissedBlocks,
  MissedCells,
  RefineOnMiss
};

bool parseCoveragePromotionMode(
    const std::string& value,
    CoveragePromotionMode* mode);
const char* toString(CoveragePromotionMode mode);

struct CoverageDrivenRefinementOptions {
  CoveragePromotionMode mode = CoveragePromotionMode::RefineOnMiss;
  std::size_t min_miss_count = 2;
  double coverage_near_band = 0.01;
};

struct CoverageDrivenRefinementResult {
  bool applied = false;
  std::string promotion_mode;
  std::vector<int> promoted_block_ids;
  std::size_t promoted_block_count = 0;
  std::size_t promoted_cell_count = 0;
  std::size_t promoted_node_count = 0;
  double resample_time_ms = 0.0;
};

class CoverageDrivenRefinement {
 public:
  static CoverageDrivenRefinementResult promoteMisses(
      const TriangleMesh& mesh,
      const CoverageAuditResult& audit,
      const CoverageDrivenRefinementOptions& options,
      AdaptiveBlockSDFModel* model,
      AdaptiveBlockSDFBuildReport* build_report,
      BlockProvenanceSet* provenance);
};

}  // namespace adasdf
