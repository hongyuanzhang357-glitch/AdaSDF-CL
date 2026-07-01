#pragma once

#include <string>

#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/recommendation/BuildRecommendationTypes.h"
#include "adasdf/recommendation/BuildSurrogateEstimator.h"

namespace adasdf {

struct BuildRecommenderOptions {
  int max_candidates = 64;
  bool include_dense = true;
  bool include_adaptive = true;
  bool include_compressed = true;
  bool emit_cli_command = true;
  SurrogateEstimatorOptions estimator_options;
};

class BuildRecommender {
 public:
  explicit BuildRecommender(
      const BuildRecommenderOptions& options =
          BuildRecommenderOptions{});

  BuildRecommendationReport recommendFromMesh(
      const TriangleMesh& mesh,
      const BuildTarget& target,
      const std::string& input_path_for_command = "model.stl",
      const std::string& output_basename = "model") const;

  BuildRecommendationReport recommendFromSTL(
      const std::string& stl_path,
      const BuildTarget& target,
      const std::string& output_basename = "model") const;

 private:
  BuildRecommenderOptions options_;
};

}  // namespace adasdf
