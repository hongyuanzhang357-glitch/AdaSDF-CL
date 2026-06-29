#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

struct BatchQueryInput {
  std::vector<Vector3> points;
};

struct BatchQueryOutput {
  std::vector<double> signed_distances;
  std::vector<Vector3> gradients;
  std::vector<Vector3> normals;
};

struct BatchQueryStats {
  std::size_t num_points = 0;
  double total_ms = 0.0;
  double ns_per_query = 0.0;
  double queries_per_second = 0.0;
  std::string backend;
};

BatchQueryOutput queryBatchCPU(
    const SDFModel& model,
    const std::vector<Vector3>& points,
    BatchQueryStats* stats = nullptr);

}  // namespace adasdf
