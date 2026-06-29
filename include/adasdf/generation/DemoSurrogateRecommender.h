#pragma once

#include <string>
#include <vector>

#include "adasdf/generation/BuildOptions.h"

namespace adasdf {

struct DemoSurrogateInput {
  std::string shape_type = "box";
  double target_near_surface_error = 1.0e-3;
  double memory_limit_mb = 64.0;
  double block_expand_limit_mb = 16.0;
  int top_k = 5;
};

struct DemoSurrogateCandidate {
  BuildOptions options;
  double predicted_p95_error = 0.0;
  double predicted_memory_mb = 0.0;
  double confidence = 0.0;
  bool credible_demo_only = false;
  std::string warning;
};

class DemoSurrogateRecommender {
 public:
  static const char* id();
  static const char* statusWarning();

  static std::vector<DemoSurrogateCandidate> recommend(
      const DemoSurrogateInput& input);
};

}  // namespace adasdf
