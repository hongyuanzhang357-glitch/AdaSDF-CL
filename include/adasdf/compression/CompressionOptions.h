#pragma once

#include "adasdf/config.h"

namespace adasdf {

enum class CompressionCodec {
  None,
  MatrixSVD,
  Tucker,
  TensorTrain,
  Auto
};

struct CompressionOptions {
  CompressionCodec codec = CompressionCodec::Tucker;
  int min_rank = 2;
  int near_surface_min_rank = 4;
  int max_rank = 16;
  int rank_step = 2;
  Scalar near_surface_error = 1e-4;
  Scalar near_band_voxels = 3.0;
  bool enable_adaptive_split = true;
  bool enable_energy_truncation = false;
  Scalar retained_energy = 0.0;
};

}  // namespace adasdf
