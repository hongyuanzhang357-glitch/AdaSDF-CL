#pragma once

#include <string>
#include <vector>

#include "adasdf/compression/CompressedSDFBlock.h"
#include "adasdf/generation/AdaptiveBlock.h"

namespace adasdf {

enum class RankSelectionMode {
  FixedRank,
  ErrorBounded,
  MemoryBounded
};

enum class CompressionSignGuardAction {
  KeepDense,
  IncreaseRank
};

bool parseCompressionSignGuardAction(
    const std::string& value,
    CompressionSignGuardAction* action);
const char* toString(CompressionSignGuardAction action);

struct BlockLowRankCompressionOptions {
  BlockCompressionMethod method = BlockCompressionMethod::MatrixSVD;

  RankSelectionMode rank_selection = RankSelectionMode::ErrorBounded;

  int fixed_rank = 4;
  int min_rank = 1;
  int max_rank = 8;

  double target_max_abs_error = 1.0e-3;
  double target_rms_error = 1.0e-4;
  double target_p95_error = 1.0e-3;

  double memory_budget_mb = 512.0;

  bool dense_fallback_if_error_exceeds_target = true;
  bool always_keep_near_surface_blocks_dense = false;
  std::vector<int> force_dense_block_ids;

  bool near_zero_compression_guard = false;
  double compression_sign_guard_band = 1.0e-2;
  double compression_near_zero_error_limit = 2.0e-3;
  CompressionSignGuardAction compression_sign_guard_action =
      CompressionSignGuardAction::KeepDense;

  double near_surface_error_weight = 1.0;

  bool verbose = true;
};

struct BlockLowRankCompressionReport {
  bool success = false;
  std::string error_message;

  std::size_t input_block_count = 0;
  std::size_t compressed_block_count = 0;
  std::size_t dense_fallback_block_count = 0;

  std::size_t near_surface_block_count = 0;

  std::size_t original_memory_bytes = 0;
  std::size_t compressed_memory_bytes = 0;

  double compression_ratio = 1.0;
  double compression_time_ms = 0.0;

  double global_max_abs_error = 0.0;
  double global_mean_abs_error = 0.0;
  double global_rms_error = 0.0;
  double global_p95_abs_error = 0.0;

  std::size_t sign_mismatch_count = 0;
  std::size_t near_surface_sign_mismatch_count = 0;

  bool compression_guard_enabled = false;
  std::size_t guarded_block_count = 0;
  std::size_t kept_dense_due_to_sign_count = 0;
  std::size_t kept_dense_due_to_error_count = 0;
  std::size_t near_zero_compression_sign_flip_count = 0;
  double near_zero_compression_p95_error = 0.0;
  std::size_t dense_fallback_memory_bytes = 0;
  std::size_t compressed_memory_bytes_after_guard = 0;

  std::vector<int> ranks_used;
  std::vector<std::string> warnings;
};

class BlockLowRankCompressor {
 public:
  static CompressedAdaptiveBlockSDF compress(
      const AdaptiveSDFBlockSet& dense_blocks,
      const BlockLowRankCompressionOptions& options,
      BlockLowRankCompressionReport* report = nullptr);
};

}  // namespace adasdf
