#include "adasdf/runtime/ActiveBlockSelector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_map>

#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"
#include "adasdf/geometry/DenseSDFModel.h"

namespace adasdf {
namespace {

double nonnegative(double value) {
  return std::isfinite(value) && value > 0.0 ? value : 0.0;
}

AABB inflate(const AABB& box, double margin) {
  if (!box.valid) {
    return box;
  }
  const double m = std::max(0.0, margin);
  return {
      {box.min.x - m, box.min.y - m, box.min.z - m},
      {box.max.x + m, box.max.y + m, box.max.z + m},
      true};
}

AABB pointBox(const Vector3& p, double margin) {
  const double m = std::max(0.0, margin);
  return {{p.x - m, p.y - m, p.z - m}, {p.x + m, p.y + m, p.z + m}, true};
}

bool intersects(const AABB& a, const AABB& b) {
  if (!a.valid || !b.valid) {
    return false;
  }
  const double eps = 1.0e-12;
  return a.min.x <= b.max.x + eps && a.max.x + eps >= b.min.x &&
         a.min.y <= b.max.y + eps && a.max.y + eps >= b.min.y &&
         a.min.z <= b.max.z + eps && a.max.z + eps >= b.min.z;
}

template <typename Block>
void selectIntersectingBlocks(
    const std::vector<Block>& blocks,
    const AABB& query_box,
    std::set<int>& selected) {
  for (const Block& block : blocks) {
    if (intersects(block.bounds, query_box)) {
      selected.insert(block.block_id);
    }
  }
}

template <typename Block>
std::unordered_map<int, AABB> blockBoundsById(const std::vector<Block>& blocks) {
  std::unordered_map<int, AABB> bounds;
  for (const Block& block : blocks) {
    bounds[block.block_id] = block.bounds;
  }
  return bounds;
}

template <typename Block>
void includeNeighbors(
    const std::vector<Block>& blocks,
    const std::set<int>& seed,
    double margin,
    std::set<int>& selected) {
  const auto bounds_by_id = blockBoundsById(blocks);
  for (const int seed_id : seed) {
    const auto it = bounds_by_id.find(seed_id);
    if (it == bounds_by_id.end()) {
      continue;
    }
    const AABB query_box = inflate(it->second, std::max(margin, 1.0e-12));
    for (const Block& block : blocks) {
      if (intersects(block.bounds, query_box)) {
        selected.insert(block.block_id);
      }
    }
  }
}

template <typename Block, typename FindContaining>
ActiveBlockSelectionResult selectFromBlocks(
    const SDFModel& model,
    const std::vector<Block>& blocks,
    FindContaining find_containing,
    const CollisionSampleSet& sample_set,
    const ActiveBlockSelectionOptions& options) {
  ActiveBlockSelectionResult result;
  result.sample_count = sample_set.size();
  result.threshold = options.threshold;
  result.selection_band = options.selection_band;
  result.extra_margin = options.extra_margin;

  std::set<int> selected;
  std::set<int> seed_selected;
  const double limit = options.threshold + options.selection_band;

  for (const CollisionSample& sample : sample_set.samples) {
    double effective_phi = 0.0;
    bool candidate = true;
    const double radius =
        options.use_sample_radius ? nonnegative(sample.radius) : 0.0;
    if (options.query_phi_for_selection) {
      const double phi = model.sampleDistance(sample.position);
      if (!std::isfinite(phi)) {
        result.error_message =
            "active block selection produced a non-finite distance.";
        return result;
      }
      effective_phi = phi - radius;
      candidate = effective_phi <= limit;
    }
    if (!candidate) {
      continue;
    }

    ++result.candidate_sample_count;
    const double margin = std::max(options.extra_margin, radius);
    if (margin > 0.0) {
      selectIntersectingBlocks(
          blocks,
          pointBox(sample.position, margin),
          seed_selected);
    } else {
      const int index = find_containing(sample.position);
      if (index >= 0) {
        seed_selected.insert(blocks[static_cast<std::size_t>(index)].block_id);
      }
    }
  }

  selected = seed_selected;
  if (options.include_neighbor_blocks && !seed_selected.empty()) {
    includeNeighbors(blocks, seed_selected, options.extra_margin, selected);
  }

  result.block_ids.assign(selected.begin(), selected.end());
  result.candidate_block_count = result.block_ids.size();
  if (options.max_active_blocks > 0 &&
      result.block_ids.size() > options.max_active_blocks) {
    result.block_ids.resize(options.max_active_blocks);
    result.warnings.push_back(
        "active block selection was truncated by max_active_blocks.");
  }
  if (result.block_ids.empty() && !sample_set.empty()) {
    result.warnings.push_back(
        "no active blocks were selected for the provided samples.");
  }
  result.success = true;
  return result;
}

}  // namespace

ActiveBlockSelectionResult ActiveBlockSelector::select(
    const SDFModel& model,
    const CollisionSampleSet& sample_set,
    const ActiveBlockSelectionOptions& options) {
  ActiveBlockSelectionResult result;
  result.sample_count = sample_set.size();
  result.threshold = options.threshold;
  result.selection_band = options.selection_band;
  result.extra_margin = options.extra_margin;

  if (!model.isValid() || !model.queryBackendAvailable()) {
    result.error_message =
        "ActiveBlockSelector requires a valid queryable SDF model.";
    return result;
  }

  if (const auto* adaptive =
          dynamic_cast<const AdaptiveBlockSDFModel*>(&model)) {
    return selectFromBlocks(
        model,
        adaptive->blockSet().blocks,
        [&](const Vector3& p) { return adaptive->findContainingBlock(p); },
        sample_set,
        options);
  }
  if (const auto* compressed =
          dynamic_cast<const CompressedAdaptiveBlockSDFModel*>(&model)) {
    return selectFromBlocks(
        model,
        compressed->compressedBlockSet().blocks,
        [&](const Vector3& p) { return compressed->findContainingBlock(p); },
        sample_set,
        options);
  }
  if (dynamic_cast<const DenseSDFModel*>(&model)) {
    result.error_message =
        "Active block selection requires an adaptive or compressed block model; "
        "dense SDF models have no block ids to activate.";
    return result;
  }

  result.error_message =
      "Active block selection requires a block-based AdaSDF model.";
  return result;
}

}  // namespace adasdf
