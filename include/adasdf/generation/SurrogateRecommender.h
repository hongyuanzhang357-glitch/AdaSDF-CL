#pragma once

#include <string>
#include <vector>

#include "adasdf/generation/BuildOptions.h"

namespace adasdf {

struct SurrogateRecommendation {
  BuildOptions options;
  double predicted_p95_error = 0.0;
  double predicted_memory_mb = 0.0;
  double safety_score = 0.0;
  bool credible = false;
  std::string note;
};

class SurrogateRecommender {
 public:
  static bool isAvailable();

  static std::vector<SurrogateRecommendation> recommend(
      const std::string& mesh_path,
      const BuildOptions& user_constraints,
      int top_k = 5);
};

}  // namespace adasdf
