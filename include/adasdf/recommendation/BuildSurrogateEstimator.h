#pragma once

#include <vector>

#include "adasdf/recommendation/BuildRecommendationTypes.h"
#include "adasdf/recommendation/SurrogateProfile.h"

namespace adasdf {

struct BuildCandidate {
  RecommendedBuildPath path = RecommendedBuildPath::AdaptiveBlockSDF;
  int dense_resolution = 0;
  int min_octree_level = 1;
  int max_octree_level = 5;
  int block_resolution = 8;
  int min_rank = 1;
  int max_rank = 8;
  int fixed_rank = 0;
  bool use_fixed_rank = false;
  bool keep_near_surface_blocks_dense = false;
  double target_refinement_error = 1.0e-3;
  double target_compression_error = 1.0e-3;
};

struct BuildEstimate {
  bool feasible = false;
  double estimated_memory_mb = 0.0;
  double estimated_near_surface_error = 0.0;
  double estimated_compression_ratio = 1.0;
  double estimated_build_cost_score = 0.0;
  double score = 0.0;
  RecommendationConfidence confidence = RecommendationConfidence::Low;
  std::vector<std::string> warnings;
  std::vector<std::string> rationale;
};

class BuildSurrogateEstimator {
 public:
  explicit BuildSurrogateEstimator(
      const SurrogateEstimatorOptions& options =
          SurrogateEstimatorOptions{});

  const SurrogateEstimatorOptions& options() const;

  BuildEstimate estimate(
      const MeshFeatureSummary& features,
      const BuildTarget& target,
      const BuildCandidate& candidate) const;

  std::vector<BuildCandidate> generateCandidates(
      const MeshFeatureSummary& features,
      const BuildTarget& target) const;

 private:
  SurrogateEstimatorOptions options_;
};

}  // namespace adasdf
