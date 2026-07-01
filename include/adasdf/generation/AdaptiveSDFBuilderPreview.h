#pragma once

#include <string>
#include <vector>

namespace adasdf {

enum class AdaptiveBuilderStage {
  MeshDiagnostics,
  MeshCleanup,
  OctreeRefinement,
  BlockPartition,
  DenseSampling,
  LowRankCompression,
  ErrorAudit,
  SDFBinWrite
};

struct AdaptiveSDFBuildOptions {
  double target_near_surface_error = 1e-3;
  double target_far_field_error = 1e-2;

  double memory_budget_mb = 512.0;

  int min_octree_level = 2;
  int max_octree_level = 8;

  int block_resolution = 32;
  int max_rank = 16;

  bool enable_octree_adaptivity = true;
  bool enable_block_adaptivity = true;
  bool enable_low_rank_compression = false;

  bool experimental = true;
};

struct AdaptiveSDFBuildPlan {
  bool valid = false;
  bool implemented_in_this_version = false;

  std::vector<AdaptiveBuilderStage> stages;
  std::vector<std::string> planned_outputs;
  std::vector<std::string> limitations;
  std::string summary;
};

class AdaptiveSDFBuilderPreview {
 public:
  static AdaptiveSDFBuildPlan makePlan(
      const AdaptiveSDFBuildOptions& options);

  static std::string planToMarkdown(
      const AdaptiveSDFBuildPlan& plan);
};

const char* toString(AdaptiveBuilderStage stage);

}  // namespace adasdf
