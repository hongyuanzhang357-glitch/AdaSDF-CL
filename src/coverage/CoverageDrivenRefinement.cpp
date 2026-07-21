#include "adasdf/coverage/CoverageDrivenRefinement.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <set>
#include <unordered_map>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/coverage/CoverageMissCluster.h"
#include "adasdf/sampling/ContactBandDiagnostics.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

Vector3 gridPoint(const AdaptiveSDFBlock& block, int i, int j, int k) {
  return {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
}

struct CellKey {
  int block_id = -1;
  int i = 0;
  int j = 0;
  int k = 0;

  bool operator<(const CellKey& other) const {
    if (block_id != other.block_id) {
      return block_id < other.block_id;
    }
    if (i != other.i) {
      return i < other.i;
    }
    if (j != other.j) {
      return j < other.j;
    }
    return k < other.k;
  }
};

std::unordered_map<int, std::size_t> blockIndexById(
    const AdaptiveBlockSDFModel& model) {
  std::unordered_map<int, std::size_t> out;
  const auto& blocks = model.blockSet().blocks;
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    out[blocks[i].block_id] = i;
  }
  return out;
}

std::set<CellKey> missedCells(
    const CoverageAuditResult& audit,
    const AdaptiveBlockSDFModel& model,
    const std::unordered_map<int, std::size_t>& block_index) {
  std::set<CellKey> out;
  for (const CoverageMissRecord& miss : audit.representative_misses) {
    const auto iter = block_index.find(miss.block_id);
    if (iter == block_index.end()) {
      continue;
    }
    const AdaptiveSDFBlock& block =
        model.blockSet().blocks[iter->second];
    const double gx =
        axisCoord(miss.point.x, block.origin.x, block.spacing.x, block.nx);
    const double gy =
        axisCoord(miss.point.y, block.origin.y, block.spacing.y, block.ny);
    const double gz =
        axisCoord(miss.point.z, block.origin.z, block.spacing.z, block.nz);
    CellKey key;
    key.block_id = miss.block_id;
    key.i = std::max(0, std::min(block.nx - 2, static_cast<int>(std::floor(gx))));
    key.j = std::max(0, std::min(block.ny - 2, static_cast<int>(std::floor(gy))));
    key.k = std::max(0, std::min(block.nz - 2, static_cast<int>(std::floor(gz))));
    out.insert(key);
  }
  return out;
}

void recomputeReportSamplingStats(
    const AdaptiveBlockSDFModel& model,
    AdaptiveBlockSDFBuildReport* report,
    const BlockProvenanceSet* provenance) {
  if (report == nullptr || provenance == nullptr) {
    return;
  }
  report->adaptive_tree_block_sampling_stats.clear();
  report->adaptive_tree_block_sampling_stats.reserve(model.blockSet().blocks.size());
  ContactBandDiagnostics diagnostics = report->contact_band_sampling;
  diagnostics.total_block_count = model.blockSet().blocks.size();
  diagnostics.contact_band_block_count = 0;
  diagnostics.far_field_block_count = 0;
  diagnostics.total_node_count = 0;
  diagnostics.exact_node_count = 0;
  diagnostics.predicted_node_count = 0;
  diagnostics.far_field_node_count = 0;
  for (const AdaptiveSDFBlock& block : model.blockSet().blocks) {
    const BlockProvenance* prov = provenance->find(block.block_id);
    const std::size_t logical =
        static_cast<std::size_t>(block.nx) *
        static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz);
    AdaptiveTreeBlockSamplingStats stats;
    stats.block_id = block.block_id;
    stats.level = block.level;
    stats.contact_band =
        prov != nullptr ? prov->is_contact_band_block : block.near_surface;
    stats.far_field = !stats.contact_band;
    stats.logical_node_count =
        prov != nullptr && prov->logical_node_count > 0
            ? prov->logical_node_count
            : logical;
    stats.exact_node_count =
        prov != nullptr ? prov->exact_node_count : (block.near_surface ? logical : 0);
    stats.predicted_node_count =
        prov != nullptr ? prov->predicted_node_count
                        : (block.near_surface ? 0 : logical);
    stats.far_field_node_count = stats.far_field ? stats.logical_node_count : 0;
    report->adaptive_tree_block_sampling_stats.push_back(stats);
    if (stats.contact_band) {
      ++diagnostics.contact_band_block_count;
    }
    if (stats.far_field) {
      ++diagnostics.far_field_block_count;
      diagnostics.far_field_node_count += stats.logical_node_count;
    }
    diagnostics.total_node_count += stats.logical_node_count;
    diagnostics.exact_node_count += stats.exact_node_count;
    diagnostics.predicted_node_count += stats.predicted_node_count;
  }
  finalizeContactBandDiagnostics(&diagnostics);
  report->contact_band_sampling = diagnostics;
}

}  // namespace

bool parseCoveragePromotionMode(
    const std::string& value,
    CoveragePromotionMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (value == "missed-blocks") {
    *mode = CoveragePromotionMode::MissedBlocks;
    return true;
  }
  if (value == "missed-cells") {
    *mode = CoveragePromotionMode::MissedCells;
    return true;
  }
  if (value == "refine-on-miss") {
    *mode = CoveragePromotionMode::RefineOnMiss;
    return true;
  }
  return false;
}

const char* toString(CoveragePromotionMode mode) {
  switch (mode) {
    case CoveragePromotionMode::MissedBlocks:
      return "missed-blocks";
    case CoveragePromotionMode::MissedCells:
      return "missed-cells";
    case CoveragePromotionMode::RefineOnMiss:
      return "refine-on-miss";
  }
  return "unknown";
}

CoverageDrivenRefinementResult CoverageDrivenRefinement::promoteMisses(
    const TriangleMesh& mesh,
    const CoverageAuditResult& audit,
    const CoverageDrivenRefinementOptions& options,
    AdaptiveBlockSDFModel* model,
    AdaptiveBlockSDFBuildReport* build_report,
    BlockProvenanceSet* provenance) {
  CoverageDrivenRefinementResult result;
  result.promotion_mode = toString(options.mode);
  if (model == nullptr || audit.missed_samples == 0) {
    return result;
  }
  const std::vector<int> block_ids =
      CoverageMissCluster::missedBlocksAboveThreshold(
          audit,
          std::max<std::size_t>(1, options.min_miss_count));
  if (block_ids.empty()) {
    return result;
  }

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = model->blockSet().signed_distance;
  sampler_options.fallback_to_bruteforce_sign = true;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options);
  const auto block_index = blockIndexById(*model);
  const auto begin = Clock::now();

  std::set<std::pair<int, std::size_t>> sampled_nodes;
  const auto sample_node = [&](AdaptiveSDFBlock& block, int i, int j, int k) {
    const std::size_t index = valueIndex(i, j, k, block.nx, block.ny);
    const auto unique_key = std::make_pair(block.block_id, index);
    if (!sampled_nodes.insert(unique_key).second) {
      return;
    }
    const BVHSDFSampleResult sample = sampler.sample(gridPoint(block, i, j, k));
    if (sample.success && std::isfinite(sample.phi)) {
      block.phi[index] = sample.phi;
    }
  };

  const bool full_block =
      options.mode == CoveragePromotionMode::MissedBlocks;
  const std::set<CellKey> cells = full_block ? std::set<CellKey>{}
                                             : missedCells(audit, *model, block_index);

  for (int block_id : block_ids) {
    const auto iter = block_index.find(block_id);
    if (iter == block_index.end()) {
      continue;
    }
    AdaptiveSDFBlock& block = model->blockSet().blocks[iter->second];
    block.near_surface = true;
    result.promoted_block_ids.push_back(block_id);
    if (full_block) {
      for (int k = 0; k < block.nz; ++k) {
        for (int j = 0; j < block.ny; ++j) {
          for (int i = 0; i < block.nx; ++i) {
            sample_node(block, i, j, k);
          }
        }
      }
      if (BlockProvenance* prov =
              provenance != nullptr ? provenance->find(block_id) : nullptr) {
        prov->is_contact_band_block = true;
        prov->is_far_field_block = false;
        prov->is_coverage_promoted = true;
        prov->has_exact_contact_cells = true;
        prov->promotion_reason = result.promotion_mode;
        prov->coverage_near_band = options.coverage_near_band;
        prov->exact_node_count = prov->logical_node_count;
        prov->predicted_node_count = 0;
      }
      continue;
    }
    std::size_t promoted_cells_for_block = 0;
    for (const CellKey& cell : cells) {
      if (cell.block_id != block_id) {
        continue;
      }
      ++promoted_cells_for_block;
      for (int dz = 0; dz <= 1; ++dz) {
        for (int dy = 0; dy <= 1; ++dy) {
          for (int dx = 0; dx <= 1; ++dx) {
            sample_node(
                block,
                std::min(cell.i + dx, block.nx - 1),
                std::min(cell.j + dy, block.ny - 1),
                std::min(cell.k + dz, block.nz - 1));
          }
        }
      }
    }
    result.promoted_cell_count += promoted_cells_for_block;
    if (BlockProvenance* prov =
            provenance != nullptr ? provenance->find(block_id) : nullptr) {
      prov->is_contact_band_block = true;
      prov->is_far_field_block = false;
      prov->is_coverage_promoted = true;
      prov->has_exact_contact_cells = true;
      prov->promotion_reason = result.promotion_mode;
      prov->coverage_near_band = options.coverage_near_band;
      const std::size_t added = promoted_cells_for_block * 8;
      prov->exact_node_count =
          std::min(prov->logical_node_count, prov->exact_node_count + added);
      prov->predicted_node_count =
          prov->logical_node_count > prov->exact_node_count
              ? prov->logical_node_count - prov->exact_node_count
              : 0;
    }
  }

  result.promoted_block_count = result.promoted_block_ids.size();
  result.promoted_node_count = sampled_nodes.size();
  result.applied = result.promoted_block_count > 0;
  const auto end = Clock::now();
  result.resample_time_ms =
      std::chrono::duration<double, std::milli>(end - begin).count();
  recomputeReportSamplingStats(*model, build_report, provenance);
  return result;
}

}  // namespace adasdf
