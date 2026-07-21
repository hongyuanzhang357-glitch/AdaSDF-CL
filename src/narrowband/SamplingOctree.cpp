#include "adasdf/narrowband/SamplingOctree.h"

#include <algorithm>
#include <array>

#include "adasdf/narrowband/SamplingDemandEstimator.h"

namespace adasdf {
namespace {

AABB paddedMeshBounds(const TriangleMesh& mesh) {
  const MeshAABB mesh_box = mesh.aabb();
  AABB box;
  box.min = toVector3(mesh_box.min);
  box.max = toVector3(mesh_box.max);
  box.valid = true;
  const Vector3 d = box.max - box.min;
  const double pad =
      std::max({d.x, d.y, d.z, 1.0}) * 0.03;
  box.min = {box.min.x - pad, box.min.y - pad, box.min.z - pad};
  box.max = {box.max.x + pad, box.max.y + pad, box.max.z + pad};
  return box;
}

Vector3 center(const AABB& box) {
  return {
      0.5 * (box.min.x + box.max.x),
      0.5 * (box.min.y + box.max.y),
      0.5 * (box.min.z + box.max.z)};
}

AABB childBox(const AABB& box, int child) {
  const Vector3 c = center(box);
  AABB out;
  out.valid = true;
  out.min = {
      (child & 1) ? c.x : box.min.x,
      (child & 2) ? c.y : box.min.y,
      (child & 4) ? c.z : box.min.z};
  out.max = {
      (child & 1) ? box.max.x : c.x,
      (child & 2) ? box.max.y : c.y,
      (child & 4) ? box.max.z : c.z};
  return out;
}

void addLeaf(
    SamplingOctreePlan* plan,
    const AABB& bounds,
    int level,
    int parent_id,
    const NarrowBandBrickBuildOptions& options,
    const SamplingDemandEstimate& estimate) {
  SamplingOctreeNode node;
  node.node_id = static_cast<int>(plan->nodes.size());
  node.level = level;
  node.bounds = bounds;
  node.parent_id = parent_id;
  node.near_zero_surface = estimate.near_zero_surface;
  node.contact_band = estimate.contact_band;
  node.far_field = estimate.far_field;
  node.zero_crossing_risk = estimate.zero_crossing_risk;
  node.high_curvature_hint = options.sampling_refine_curvature_hint &&
      options.sampling_curvature_aware;
  node.small_gap_hint = options.sampling_refine_small_gap_hint &&
      options.sampling_small_gap_aware;
  node.target_sample_spacing = estimate.target_sample_spacing;
  node.exact_sample_required = estimate.exact_required;
  node.exact_required = estimate.exact_required;
  node.can_be_interpolated = !estimate.exact_required;
  node.estimated_exact_sample_count = estimate.estimated_exact_sample_count;
  node.estimated_interpolated_sample_count =
      estimate.estimated_interpolated_sample_count;
  ++plan->node_count_by_level[level];
  plan->exact_sample_estimate_by_level[level] +=
      node.estimated_exact_sample_count;
  plan->interpolated_sample_estimate_by_level[level] +=
      node.estimated_interpolated_sample_count;
  if (node.contact_band) {
    ++plan->contact_band_node_count_by_level[level];
  } else {
    ++plan->far_field_node_count_by_level[level];
  }
  plan->nodes.push_back(std::move(node));
}

void recurse(
    SamplingOctreePlan* plan,
    const AABB& bounds,
    int level,
    int parent_id,
    const BVHSDFSampler& sampler,
    const NarrowBandBrickBuildOptions& options) {
  const SamplingDemandEstimate estimate =
      SamplingDemandEstimator::estimate(bounds, level, sampler, options);
  if (!estimate.should_refine) {
    addLeaf(plan, bounds, level, parent_id, options, estimate);
    return;
  }
  const int internal_id = -static_cast<int>(plan->nodes.size()) - 1;
  for (int child = 0; child < 8; ++child) {
    recurse(
        plan,
        childBox(bounds, child),
        level + 1,
        internal_id,
        sampler,
        options);
  }
}

}  // namespace

SamplingOctreePlan SamplingOctree::build(
    const TriangleMesh& mesh,
    const BVHSDFSampler& sampler,
    const NarrowBandBrickBuildOptions& options) {
  SamplingOctreePlan plan;
  plan.bounds = paddedMeshBounds(mesh);
  recurse(&plan, plan.bounds, 0, -1, sampler, options);
  return plan;
}

}  // namespace adasdf
