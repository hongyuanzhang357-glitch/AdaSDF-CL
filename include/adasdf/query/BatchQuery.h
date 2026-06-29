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

struct BatchQueryTiming {
  double setup_ms = 0.0;
  double expand_ms = 0.0;
  double upload_sdf_ms = 0.0;

  double h2d_points_ms = 0.0;
  double kernel_ms = 0.0;
  double d2h_results_ms = 0.0;
  double sync_ms = 0.0;
  double postprocess_ms = 0.0;

  double allocation_ms = 0.0;
  double free_ms = 0.0;

  double total_ms = 0.0;
};

struct BatchQueryStats {
  std::size_t num_points = 0;
  double total_ms = 0.0;
  double ns_per_query = 0.0;
  double queries_per_second = 0.0;
  std::string backend;
  BatchQueryTiming timing;
};

BatchQueryOutput queryBatchCPU(
    const SDFModel& model,
    const std::vector<Vector3>& points,
    BatchQueryStats* stats = nullptr,
    BatchQueryTiming* timing = nullptr);

}  // namespace adasdf
