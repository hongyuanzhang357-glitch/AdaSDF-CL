#include "adasdf/generation/AdaptiveSDFBuilderPreview.h"

#include <sstream>

namespace adasdf {

const char* toString(AdaptiveBuilderStage stage) {
  switch (stage) {
    case AdaptiveBuilderStage::MeshDiagnostics:
      return "MeshDiagnostics";
    case AdaptiveBuilderStage::MeshCleanup:
      return "MeshCleanup";
    case AdaptiveBuilderStage::OctreeRefinement:
      return "OctreeRefinement";
    case AdaptiveBuilderStage::BlockPartition:
      return "BlockPartition";
    case AdaptiveBuilderStage::DenseSampling:
      return "DenseSampling";
    case AdaptiveBuilderStage::LowRankCompression:
      return "LowRankCompression";
    case AdaptiveBuilderStage::ErrorAudit:
      return "ErrorAudit";
    case AdaptiveBuilderStage::SDFBinWrite:
      return "SDFBinWrite";
  }
  return "Unknown";
}

AdaptiveSDFBuildPlan AdaptiveSDFBuilderPreview::makePlan(
    const AdaptiveSDFBuildOptions& options) {
  AdaptiveSDFBuildPlan plan;
  plan.valid = options.target_near_surface_error > 0.0 &&
               options.target_far_field_error > 0.0 &&
               options.memory_budget_mb > 0.0 &&
               options.min_octree_level >= 0 &&
               options.max_octree_level >= options.min_octree_level &&
               options.block_resolution >= 2 &&
               options.max_rank >= 1;
  plan.implemented_in_this_version = true;
  plan.summary =
      "Adaptive octree/block SDF construction is implemented in "
      "v1.6.0-alpha as block-wise dense SDF output. Low-rank compression "
      "remains planned work.";

  plan.stages = {
      AdaptiveBuilderStage::MeshDiagnostics,
      AdaptiveBuilderStage::MeshCleanup,
      AdaptiveBuilderStage::OctreeRefinement,
      AdaptiveBuilderStage::BlockPartition,
      AdaptiveBuilderStage::DenseSampling,
      AdaptiveBuilderStage::LowRankCompression,
      AdaptiveBuilderStage::ErrorAudit,
      AdaptiveBuilderStage::SDFBinWrite};
  plan.planned_outputs = {
      "adaptive octree metadata",
      "block partition metadata",
      "dense sampled block payloads",
      "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1 block-wise dense .sdfbin"};
  plan.implemented_stages = {
      "OctreeRefinement implemented in v1.6.0-alpha",
      "BlockPartition implemented in v1.6.0-alpha",
      "DenseSampling implemented in v1.6.0-alpha"};
  plan.planned_stages = {
      "LowRankCompression planned for v1.7.0-alpha",
      "SurrogateRecommendation planned for v1.8.0-alpha"};
  plan.limitations = {
      "The real v1.6 builder writes block-wise dense adaptive SDF data.",
      "No SVD, Tucker, or low-rank compression is executed.",
      "No surrogate-guided parameter recommendation is executed.",
      "No output .sdfbin is written by the preview CLI."};
  if (!options.enable_low_rank_compression) {
    plan.limitations.push_back(
        "Low-rank compression is disabled in the requested preview options.");
  }
  if (!plan.valid) {
    plan.limitations.push_back(
        "Requested preview options are internally inconsistent.");
  }
  return plan;
}

std::string AdaptiveSDFBuilderPreview::planToMarkdown(
    const AdaptiveSDFBuildPlan& plan) {
  std::ostringstream out;
  out << "# Adaptive SDF Builder Preview Plan\n\n";
  out << plan.summary << "\n\n";
  out << "- Valid options: " << (plan.valid ? "yes" : "no") << "\n";
  out << "- Implemented in this version: "
      << (plan.implemented_in_this_version ? "yes" : "no") << "\n\n";
  out << "## Planned Stages\n\n";
  for (const AdaptiveBuilderStage stage : plan.stages) {
    out << "- " << toString(stage) << "\n";
  }
  out << "\n## Implemented Stage Status\n\n";
  for (const std::string& stage : plan.implemented_stages) {
    out << "- " << stage << "\n";
  }
  out << "\n## Planned Stage Status\n\n";
  for (const std::string& stage : plan.planned_stages) {
    out << "- " << stage << "\n";
  }
  out << "\n## Planned Outputs\n\n";
  for (const std::string& output : plan.planned_outputs) {
    out << "- " << output << "\n";
  }
  out << "\n## Limitations\n\n";
  for (const std::string& limitation : plan.limitations) {
    out << "- " << limitation << "\n";
  }
  out << "\nAdaptive octree/block SDF construction is implemented in v1.6.0-alpha. "
         "Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF "
         "construction. This preview intentionally does not write a .sdfbin; "
         "low-rank compression is planned for v1.7.0-alpha.\n";
  return out.str();
}

}  // namespace adasdf
