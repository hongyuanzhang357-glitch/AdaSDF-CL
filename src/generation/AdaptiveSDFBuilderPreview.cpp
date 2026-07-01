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
    case AdaptiveBuilderStage::TuckerCompression:
      return "TuckerCompression";
    case AdaptiveBuilderStage::SurrogateRecommendation:
      return "SurrogateRecommendation";
    case AdaptiveBuilderStage::GPUCompressedQuery:
      return "GPUCompressedQuery";
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
      "Adaptive octree/block SDF construction and matrix-SVD low-rank block "
      "compression are implemented in v1.7.0-alpha. Tucker/HOSVD compression "
      "and surrogate-guided recommendation remain planned work.";

  plan.stages = {
      AdaptiveBuilderStage::MeshDiagnostics,
      AdaptiveBuilderStage::MeshCleanup,
      AdaptiveBuilderStage::OctreeRefinement,
      AdaptiveBuilderStage::BlockPartition,
      AdaptiveBuilderStage::DenseSampling,
      AdaptiveBuilderStage::LowRankCompression,
      AdaptiveBuilderStage::TuckerCompression,
      AdaptiveBuilderStage::SurrogateRecommendation,
      AdaptiveBuilderStage::GPUCompressedQuery,
      AdaptiveBuilderStage::ErrorAudit,
      AdaptiveBuilderStage::SDFBinWrite};
  plan.planned_outputs = {
      "adaptive octree metadata",
      "block partition metadata",
      "dense sampled block payloads",
      "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1 block-wise dense .sdfbin",
      "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1 matrix-SVD compressed .sdfbin"};
  plan.implemented_stages = {
      "OctreeRefinement implemented in v1.6.0-alpha",
      "BlockPartition implemented in v1.6.0-alpha",
      "DenseSampling implemented in v1.6.0-alpha",
      "LowRankCompression implemented in v1.7.0-alpha using matrix-SVD"};
  plan.planned_stages = {
      "TuckerCompression planned",
      "SurrogateRecommendation planned for v1.8.0-alpha",
      "GPUCompressedQuery planned"};
  plan.limitations = {
      "The v1.7 builder supports matrix-SVD block compression and dense fallback.",
      "Tucker/HOSVD compression is not implemented in this release.",
      "No surrogate-guided parameter recommendation is executed.",
      "No GPU-native compressed query is executed.",
      "No output .sdfbin is written by the preview CLI."};
  if (!options.enable_low_rank_compression) {
    plan.limitations.push_back(
        "Low-rank compression can be enabled in real builders with "
        "adasdf_build_compressed_sdf or --enable-low-rank.");
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
         "Low-rank block compression is implemented in v1.7.0-alpha using "
         "matrix-SVD per adaptive block. Tucker/HOSVD compression is not "
         "implemented in this release. Use adasdf_build_compressed_sdf for "
         "one-step STL-to-compressed-SDF construction. This preview "
         "intentionally does not write a .sdfbin.\n";
  return out.str();
}

}  // namespace adasdf
