#pragma once

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"
#include "adasdf/narrowband/NarrowBandBrickBuildStats.h"

namespace adasdf {

struct ContactWeightedCompressionReport {
  std::size_t compressed_block_count = 0;
  std::size_t dense_fallback_block_count = 0;
  std::size_t contact_band_sign_flip_count = 0;
  double contact_band_p95_compression_error = 0.0;
  int rank_min = 0;
  double rank_mean = 0.0;
  int rank_max = 0;
  std::size_t estimated_compressed_bytes = 0;
  std::size_t estimated_expanded_bytes = 0;
};

class ContactWeightedCompressor {
 public:
  static AdaptiveSDFBlockSet compressCompatible(
      AdaptiveSDFBlockSet dense_blocks,
      const NarrowBandBrickBuildOptions& options,
      ContactWeightedCompressionReport* report);
};

}  // namespace adasdf
