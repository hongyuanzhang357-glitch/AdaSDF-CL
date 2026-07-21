#include "adasdf/narrowband/ContactWeightedCompressor.h"

#include <algorithm>

namespace adasdf {

AdaptiveSDFBlockSet ContactWeightedCompressor::compressCompatible(
    AdaptiveSDFBlockSet dense_blocks,
    const NarrowBandBrickBuildOptions& options,
    ContactWeightedCompressionReport* report) {
  ContactWeightedCompressionReport local;
  for (const AdaptiveSDFBlock& block : dense_blocks.blocks) {
    const std::size_t bytes = block.phi.size() * sizeof(double);
    local.estimated_expanded_bytes += bytes;
    local.estimated_compressed_bytes += bytes;
  }
  local.rank_min = dense_blocks.blocks.empty() ? 0 : std::max(1, options.max_rank);
  local.rank_max = dense_blocks.blocks.empty() ? 0 : std::max(1, options.max_rank);
  local.rank_mean =
      dense_blocks.blocks.empty() ? 0.0 : static_cast<double>(local.rank_max);

  // Prototype policy: keep the emitted file in the existing adaptive dense
  // SDFBin format. This preserves old readers and query/collide APIs while the
  // contact-weighted low-rank format remains experimental planning metadata.
  local.compressed_block_count = 0;
  local.dense_fallback_block_count = dense_blocks.blocks.size();
  local.contact_band_sign_flip_count = 0;
  local.contact_band_p95_compression_error = 0.0;
  if (report != nullptr) {
    *report = local;
  }
  return dense_blocks;
}

}  // namespace adasdf
