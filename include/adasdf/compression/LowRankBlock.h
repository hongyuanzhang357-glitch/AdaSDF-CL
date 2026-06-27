#pragma once

#include <cstdint>
#include <vector>

#include "adasdf/compression/CompressionOptions.h"
#include "adasdf/geometry/Transform.h"

namespace adasdf {

using BlockId = std::int32_t;

struct VoxelResolution {
  int x = 0;
  int y = 0;
  int z = 0;
};

struct LowRankPayload {
  CompressionCodec codec = CompressionCodec::Tucker;
  int rank_x = 0;
  int rank_y = 0;
  int rank_z = 0;

  // Placeholder for SVD/Tucker/TT factors. Future code should map this to
  // sdf::LowRankBlock without duplicating the existing compression kernels.
  std::vector<Scalar> basis;
  std::vector<Scalar> coefficients;
};

struct LowRankBlock {
  BlockId block_id = -1;
  Vector3 origin;
  Vector3 local_min;
  Vector3 local_max;
  Scalar voxel_size = 0.0;
  VoxelResolution resolution;
  LowRankPayload payload;
  Scalar max_reconstruction_error = 0.0;
  Scalar near_surface_error = 0.0;
  bool near_surface = false;
  bool sign_change = false;
};

}  // namespace adasdf
