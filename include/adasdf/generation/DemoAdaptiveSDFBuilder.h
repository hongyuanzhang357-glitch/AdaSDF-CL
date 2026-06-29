#pragma once

#include <memory>
#include <string>

#include "adasdf/generation/DemoSurrogateRecommender.h"
#include "adasdf/geometry/DemoAdaptiveSDFModel.h"

namespace adasdf {

struct DemoAdaptiveBuildRequest {
  std::string shape_type = "box";
  Vector3 center = Vector3::Zero();
  Vector3 half_extent{0.5, 0.5, 0.5};
  std::string unit = "m";
  double target_near_surface_error = 1.0e-3;
  double memory_limit_mb = 64.0;
  double block_expand_limit_mb = 16.0;
  bool use_surrogate = true;
  int top_k = 5;
};

struct DemoAdaptiveBuildResult {
  std::shared_ptr<DemoAdaptiveSDFModel> model;
  DemoSurrogateCandidate candidate;
  std::string warning;
};

class DemoAdaptiveSDFBuilder {
 public:
  static DemoAdaptiveBuildResult build(const DemoAdaptiveBuildRequest& request);
};

}  // namespace adasdf
