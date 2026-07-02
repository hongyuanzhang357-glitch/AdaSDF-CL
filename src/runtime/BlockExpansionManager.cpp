#include "adasdf/runtime/BlockExpansionManager.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <utility>

#include "adasdf/compression/CompressedSDFBlock.h"
#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {
namespace {

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

ActiveExpandedBlock fromAdaptiveBlock(const AdaptiveSDFBlock& block) {
  ActiveExpandedBlock expanded;
  expanded.block_id = block.block_id;
  expanded.source_block_id = block.block_id;
  expanded.level = block.level;
  expanded.bounds = block.bounds;
  expanded.nx = block.nx;
  expanded.ny = block.ny;
  expanded.nz = block.nz;
  expanded.origin = block.origin;
  expanded.spacing = block.spacing;
  expanded.near_surface = block.near_surface;
  expanded.signed_distance = block.signed_distance;
  expanded.phi = block.phi;
  return expanded;
}

ActiveExpandedBlock fromCompressedBlock(const CompressedSDFBlock& block) {
  ActiveExpandedBlock expanded;
  expanded.block_id = block.block_id;
  expanded.source_block_id =
      block.source_block_id >= 0 ? block.source_block_id : block.block_id;
  expanded.level = block.level;
  expanded.bounds = block.bounds;
  expanded.nx = block.nx;
  expanded.ny = block.ny;
  expanded.nz = block.nz;
  expanded.origin = block.origin;
  expanded.spacing = block.spacing;
  expanded.near_surface = block.near_surface;
  expanded.signed_distance = block.signed_distance;
  expanded.phi.resize(
      static_cast<std::size_t>(block.nx) *
      static_cast<std::size_t>(block.ny) *
      static_cast<std::size_t>(block.nz));
  for (int k = 0; k < block.nz; ++k) {
    for (int j = 0; j < block.ny; ++j) {
      for (int i = 0; i < block.nx; ++i) {
        expanded.phi[valueIndex(i, j, k, block.nx, block.ny)] =
            compressedBlockGridValue(block, i, j, k);
      }
    }
  }
  return expanded;
}

std::vector<int> sortedUnique(const std::vector<int>& ids) {
  std::set<int> unique(ids.begin(), ids.end());
  return {unique.begin(), unique.end()};
}

}  // namespace

BlockExpansionManager::BlockExpansionManager(ExpandedBlockCache* cache)
    : cache_(cache) {}

BlockExpansionResult BlockExpansionManager::ensureBlocksExpanded(
    const SDFModel& model,
    const std::vector<int>& block_ids) {
  const auto start = std::chrono::steady_clock::now();
  BlockExpansionResult result;
  if (!cache_) {
    result.error_message = "BlockExpansionManager requires a cache.";
    return result;
  }
  if (!model.isValid() || !model.queryBackendAvailable()) {
    result.error_message =
        "BlockExpansionManager requires a valid queryable SDF model.";
    return result;
  }

  const std::vector<int> requested = sortedUnique(block_ids);
  result.stats.requested_block_count = requested.size();
  for (const int block_id : requested) {
    if (cache_->contains(block_id)) {
      ++result.stats.cache_hit_count;
      continue;
    }
    ++result.stats.cache_miss_count;
    std::string error;
    ActiveExpandedBlock expanded = expandBlock(model, block_id, &error);
    if (!expanded.isValid()) {
      ++result.stats.skipped_block_count;
      result.warnings.push_back(
          error.empty() ? "failed to expand requested block." : error);
      continue;
    }
    result.stats.expanded_memory_bytes += expanded.memoryBytes();
    cache_->put(std::move(expanded));
    ++result.stats.expanded_block_count;
  }

  result.resident_block_ids = cache_->residentBlockIds();
  result.cache_stats = cache_->stats();
  result.stats.expansion_time_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  result.success = true;
  return result;
}

ActiveExpandedBlock BlockExpansionManager::expandBlock(
    const SDFModel& model,
    int block_id,
    std::string* error_message) const {
  if (const auto* adaptive =
          dynamic_cast<const AdaptiveBlockSDFModel*>(&model)) {
    for (const AdaptiveSDFBlock& block : adaptive->blockSet().blocks) {
      if (block.block_id == block_id) {
        return fromAdaptiveBlock(block);
      }
    }
    if (error_message) {
      *error_message =
          "adaptive block id not found: " + std::to_string(block_id);
    }
    return {};
  }

  if (const auto* compressed =
          dynamic_cast<const CompressedAdaptiveBlockSDFModel*>(&model)) {
    for (const CompressedSDFBlock& block :
         compressed->compressedBlockSet().blocks) {
      if (block.block_id == block_id) {
        return fromCompressedBlock(block);
      }
    }
    if (error_message) {
      *error_message =
          "compressed block id not found: " + std::to_string(block_id);
    }
    return {};
  }

  if (error_message) {
    *error_message =
        "active block expansion requires an adaptive or compressed block model.";
  }
  return {};
}

}  // namespace adasdf
