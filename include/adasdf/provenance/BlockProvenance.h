#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {

struct BlockProvenance {
  int block_id = -1;
  int level = -1;

  bool is_contact_band_block = false;
  bool is_far_field_block = false;
  bool is_coverage_promoted = false;
  bool has_exact_contact_cells = false;

  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t logical_node_count = 0;

  std::string sampling_mode;
  std::string marker_mode;
  std::string promotion_reason;

  double contact_band_width = 0.0;
  double coverage_near_band = 0.0;
};

struct BlockProvenanceSummary {
  std::size_t block_count = 0;
  std::size_t contact_band_block_count = 0;
  std::size_t far_field_block_count = 0;
  std::size_t coverage_promoted_block_count = 0;
  std::size_t exact_node_total = 0;
  std::size_t predicted_node_total = 0;
  std::size_t logical_node_total = 0;
};

struct BlockProvenanceSet {
  std::string schema_id = "adasdf.block_provenance.v1";
  std::string provenance_format = "sidecar_json";
  std::filesystem::path sdf_path;
  std::vector<BlockProvenance> blocks;

  const BlockProvenance* find(int block_id) const;
  BlockProvenance* find(int block_id);
  BlockProvenanceSummary summary() const;
};

class BlockProvenanceIO {
 public:
  static std::filesystem::path defaultSidecarPath(
      const std::filesystem::path& sdf_path);

  static BlockProvenanceSet fromAdaptiveModel(
      const AdaptiveBlockSDFModel& model,
      const AdaptiveBlockSDFBuildReport& build_report,
      const std::string& sampling_mode,
      double contact_band_width,
      double coverage_near_band);

  static BlockProvenanceSet fromCompressedModel(
      const CompressedAdaptiveBlockSDFModel& model,
      const AdaptiveBlockSDFBuildReport& build_report,
      const std::string& sampling_mode,
      double contact_band_width,
      double coverage_near_band);

  static void markCoveragePromoted(
      BlockProvenanceSet* provenance,
      const std::vector<int>& block_ids,
      const std::string& reason,
      double coverage_near_band);

  static bool writeJson(
      const std::filesystem::path& path,
      const BlockProvenanceSet& provenance,
      std::string* error = nullptr);

  static bool readJson(
      const std::filesystem::path& path,
      BlockProvenanceSet* provenance,
      std::string* error = nullptr);

  static bool readSidecarForSDF(
      const std::filesystem::path& sdf_path,
      BlockProvenanceSet* provenance,
      std::string* error = nullptr);

  static std::string summaryJson(const BlockProvenanceSet& provenance);
};

}  // namespace adasdf
