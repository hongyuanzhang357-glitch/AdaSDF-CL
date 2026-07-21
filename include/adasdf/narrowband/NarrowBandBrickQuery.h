#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/narrowband/NarrowBandBrickIndex.h"

namespace adasdf {

struct NarrowBandBrickQueryBreakdown {
  double block_lookup_time_ms = 0.0;
  double cell_index_time_ms = 0.0;
  double interpolation_time_ms = 0.0;
  double model_dispatch_time_ms = 0.0;
  double decompression_or_decode_time_ms = 0.0;
  double data_access_time_ms = 0.0;
};

struct NarrowBandBrickQueryStats {
  std::size_t sample_count = 0;
  std::size_t block_lookup_count = 0;
  std::size_t block_lookup_miss_count = 0;
  std::size_t out_of_domain_count = 0;
  std::size_t blocks_checked = 0;
  std::size_t tensor_nodes_touched = 0;
  double phi_min = 0.0;
  double phi_max = 0.0;
  double phi_mean = 0.0;
  double phi_sum = 0.0;
  NarrowBandBrickQueryBreakdown timing;

  double averageBlocksCheckedPerQuery() const;
  double averageTensorNodesTouchedPerQuery() const;
};

struct NarrowBandBrickQueryResult {
  bool success = false;
  bool out_of_domain = false;
  int brick_id = -1;
  double phi = 0.0;
};

class NarrowBandBrickQuery {
 public:
  static NarrowBandBrickQueryResult samplePhi(
      const NarrowBandBrickIndex& index,
      const Vector3& point,
      NarrowBandBrickQueryStats* stats = nullptr);

  static std::vector<double> queryPhi(
      const NarrowBandBrickIndex& index,
      const std::vector<Vector3>& points,
      NarrowBandBrickQueryStats* stats = nullptr);
};

}  // namespace adasdf

