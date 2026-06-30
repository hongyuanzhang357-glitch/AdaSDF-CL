#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

enum class QueryOutputMode {
  PhiOnly,
  PhiAndNormal
};

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

  bool workspace_reused = false;
  std::size_t allocation_count = 0;
  std::size_t workspace_capacity = 0;
  double workspace_device_memory_mb = 0.0;

  std::size_t block_lookup_count = 0;
  std::size_t block_scan_count = 0;
  double center_block_hit_rate = 0.0;
  double neighbor_same_block_rate = 0.0;

  bool download_results = true;
  bool correctness_checked = true;
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

BatchQueryOutput queryBatchCPU(
    const SDFModel& model,
    const std::vector<Vector3>& points,
    QueryOutputMode output_mode,
    BatchQueryStats* stats = nullptr,
    BatchQueryTiming* timing = nullptr);

const char* toString(QueryOutputMode output_mode);
bool includesNormals(QueryOutputMode output_mode);

}  // namespace adasdf
