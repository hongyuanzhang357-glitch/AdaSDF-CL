#include "adasdf/generation/DemoAdaptiveSDFBuilder.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace adasdf {
namespace {

double clampDouble(double value, double lo, double hi) {
  return std::max(lo, std::min(value, hi));
}

Vector3 lerp(const Vector3& a, const Vector3& b, double t) {
  return {
      a.x + (b.x - a.x) * t,
      a.y + (b.y - a.y) * t,
      a.z + (b.z - a.z) * t};
}

std::vector<DemoOctreeNodeInfo> makeOctreeNodes(
    const DemoAdaptiveSDFDescription& description,
    const BuildOptions& options) {
  const Vector3 min_corner = description.center - description.half_extent;
  const Vector3 max_corner = description.center + description.half_extent;
  std::vector<DemoOctreeNodeInfo> nodes;
  nodes.push_back({0, min_corner, max_corner, true});

  const int level = std::max(2, options.max_octree_level);
  const int shells = std::min(level, 6);
  for (int i = 1; i <= shells; ++i) {
    const double shrink = 0.05 * static_cast<double>(i);
    const Vector3 inner_min = lerp(min_corner, description.center, shrink);
    const Vector3 inner_max = lerp(max_corner, description.center, shrink);
    nodes.push_back({i, inner_min, inner_max, i >= shells - 1});
  }
  return nodes;
}

std::vector<DemoBlockInfo> makeBlocks(
    const DemoAdaptiveSDFDescription& description,
    const BuildOptions& options) {
  const int resolution = std::max(8, options.base_block_cells);
  const int base_rank = std::max(2, options.max_rank);
  const int memory_limited_blocks = static_cast<int>(
      clampDouble(description.memory_limit_mb / 8.0, 2.0, 12.0));
  const int level_blocks = std::max(4, options.max_octree_level + 2);
  const int block_count = std::min(memory_limited_blocks, level_blocks);

  const Vector3 min_corner = description.center - description.half_extent;
  const Vector3 max_corner = description.center + description.half_extent;
  std::vector<DemoBlockInfo> blocks;
  blocks.reserve(static_cast<std::size_t>(block_count));
  for (int i = 0; i < block_count; ++i) {
    const double t0 = static_cast<double>(i) / static_cast<double>(block_count);
    const double t1 = static_cast<double>(i + 1) / static_cast<double>(block_count);
    DemoBlockInfo block;
    block.block_id = i;
    block.min_corner = {min_corner.x + (max_corner.x - min_corner.x) * t0,
                        min_corner.y,
                        min_corner.z};
    block.max_corner = {min_corner.x + (max_corner.x - min_corner.x) * t1,
                        max_corner.y,
                        max_corner.z};
    block.resolution = resolution;
    block.rank = std::max(2, base_rank - (i % 3));
    block.estimated_error =
        description.target_near_surface_error * (1.0 + 0.05 * (i % 4));
    block.estimated_memory_kb =
        static_cast<double>(resolution * resolution * block.rank) / 8.0;
    blocks.push_back(block);
  }
  return blocks;
}

}  // namespace

DemoAdaptiveBuildResult DemoAdaptiveSDFBuilder::build(
    const DemoAdaptiveBuildRequest& request) {
  if (request.shape_type != "box") {
    throw std::runtime_error(
        "DemoAdaptiveSDFBuilder currently supports shape_type=box only.");
  }
  if (!request.center.allFinite() || !request.half_extent.allFinite()) {
    throw std::runtime_error("DemoAdaptiveSDFBuilder box parameters must be finite.");
  }
  if (!(request.half_extent.x > 0.0) ||
      !(request.half_extent.y > 0.0) ||
      !(request.half_extent.z > 0.0)) {
    throw std::runtime_error(
        "DemoAdaptiveSDFBuilder box half extents must be positive.");
  }

  DemoSurrogateInput input;
  input.shape_type = request.shape_type;
  input.target_near_surface_error = request.target_near_surface_error;
  input.memory_limit_mb = request.memory_limit_mb;
  input.block_expand_limit_mb = request.block_expand_limit_mb;
  input.top_k = request.top_k;

  DemoSurrogateCandidate candidate;
  if (request.use_surrogate) {
    candidate = DemoSurrogateRecommender::recommend(input).front();
  } else {
    candidate.options.near_surface_error = request.target_near_surface_error;
    candidate.options.tau_near_abs = request.target_near_surface_error;
    candidate.options.memory_limit_mb = request.memory_limit_mb;
    candidate.options.block_memory_limit_mb = request.block_expand_limit_mb;
    candidate.options.max_memory_mb =
        static_cast<std::size_t>(std::ceil(request.memory_limit_mb));
    candidate.options.block_expand_limit_mb =
        static_cast<std::size_t>(std::ceil(request.block_expand_limit_mb));
    candidate.options.max_octree_level = 4;
    candidate.options.base_block_cells = 24;
    candidate.options.max_rank = 6;
    candidate.predicted_p95_error = request.target_near_surface_error * 1.25;
    candidate.predicted_memory_mb = std::min(4.0, request.memory_limit_mb);
    candidate.confidence = 0.5;
    candidate.credible_demo_only = true;
    candidate.warning = DemoSurrogateRecommender::statusWarning();
  }

  DemoAdaptiveSDFDescription description;
  description.shape_type = request.shape_type;
  description.center = request.center;
  description.half_extent = request.half_extent;
  description.unit = request.unit;
  description.target_near_surface_error = request.target_near_surface_error;
  description.memory_limit_mb = request.memory_limit_mb;
  description.block_expand_limit_mb = request.block_expand_limit_mb;
  description.surrogate_id = DemoSurrogateRecommender::id();
  description.warning = "demo_surrogate_not_universal_not_fully_trained";
  description.build_options = candidate.options;

  auto octree_nodes = makeOctreeNodes(description, candidate.options);
  auto blocks = makeBlocks(description, candidate.options);

  DemoAdaptiveBuildResult result;
  result.candidate = candidate;
  result.warning = candidate.warning;
  result.model = DemoAdaptiveSDFModel::create(
      description,
      std::move(octree_nodes),
      std::move(blocks));
  return result;
}

}  // namespace adasdf
