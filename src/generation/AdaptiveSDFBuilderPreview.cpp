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
  plan.implemented_in_this_version = false;
  plan.summary =
      "Adaptive compressed builder is interface preview only in "
      "v1.5.0-alpha.";

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
      "low-rank compressed block payloads",
      "adaptive compressed .sdfbin"};
  plan.limitations = {
      "Full adaptive compressed SDF build is not implemented in v1.5.0-alpha.",
      "No octree refinement algorithm is executed.",
      "No block builder is executed.",
      "No SVD, Tucker, or low-rank compression is executed.",
      "No surrogate-guided parameter recommendation is executed.",
      "No output .sdfbin is written by the preview."};
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
  out << "\n## Planned Outputs\n\n";
  for (const std::string& output : plan.planned_outputs) {
    out << "- " << output << "\n";
  }
  out << "\n## Limitations\n\n";
  for (const std::string& limitation : plan.limitations) {
    out << "- " << limitation << "\n";
  }
  out << "\nThis preview intentionally does not write an adaptive compressed "
         "SDF file in v1.5.0-alpha.\n";
  return out.str();
}

}  // namespace adasdf
