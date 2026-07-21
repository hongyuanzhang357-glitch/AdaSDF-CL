#include "adasdf/compression/BlockLowRankCompressor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <unordered_set>

#include "adasdf/compression/SmallMatrixSVD.h"

namespace adasdf {
namespace {

std::size_t denseIndex(int ix, int iy, int iz, int nx, int ny) {
  return static_cast<std::size_t>(ix) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(iy) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(iz));
}

std::size_t matrixIndex(int row, int col, int cols) {
  return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
         static_cast<std::size_t>(col);
}

double percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const double clamped = std::clamp(q, 0.0, 1.0);
  const std::size_t index = static_cast<std::size_t>(
      std::round(clamped * static_cast<double>(values.size() - 1)));
  return values[index];
}

std::vector<double> blockToMatrix(const AdaptiveSDFBlock& block) {
  const int rows = block.nx;
  const int cols = block.ny * block.nz;
  std::vector<double> matrix(static_cast<std::size_t>(rows) *
                                 static_cast<std::size_t>(cols),
                             0.0);
  for (int ix = 0; ix < block.nx; ++ix) {
    for (int iy = 0; iy < block.ny; ++iy) {
      for (int iz = 0; iz < block.nz; ++iz) {
        const int col = iy * block.nz + iz;
        matrix[matrixIndex(ix, col, cols)] =
            block.phi[denseIndex(ix, iy, iz, block.nx, block.ny)];
      }
    }
  }
  return matrix;
}

struct ErrorStats {
  double max_abs = 0.0;
  double mean_abs = 0.0;
  double rms = 0.0;
  double p95 = 0.0;
  std::size_t sign_mismatch = 0;
  std::size_t near_surface_sign_mismatch = 0;
  std::vector<double> abs_errors;
};

struct NearZeroGuardStats {
  std::size_t sample_count = 0;
  std::size_t sign_flip_count = 0;
  double max_abs_error = 0.0;
  double p95_abs_error = 0.0;
  std::vector<double> abs_errors;
};

bool signMismatch(double a, double b) {
  if (std::abs(a) <= 1.0e-12 || std::abs(b) <= 1.0e-12) {
    return false;
  }
  return (a < 0.0) != (b < 0.0);
}

ErrorStats computeStats(
    const AdaptiveSDFBlock& reference,
    const SmallSVDResult& svd) {
  ErrorStats stats;
  double sum_abs = 0.0;
  double sum_sq = 0.0;
  const std::size_t count =
      static_cast<std::size_t>(reference.nx) *
      static_cast<std::size_t>(reference.ny) *
      static_cast<std::size_t>(reference.nz);
  stats.abs_errors.reserve(count);

  for (int ix = 0; ix < reference.nx; ++ix) {
    for (int iy = 0; iy < reference.ny; ++iy) {
      for (int iz = 0; iz < reference.nz; ++iz) {
        const int col = iy * reference.nz + iz;
        const double dense =
            reference.phi[denseIndex(ix, iy, iz, reference.nx, reference.ny)];
        const double compressed = SmallMatrixSVD::reconstructValue(svd, ix, col);
        const double error = compressed - dense;
        const double abs_error = std::abs(error);
        stats.abs_errors.push_back(abs_error);
        stats.max_abs = std::max(stats.max_abs, abs_error);
        sum_abs += abs_error;
        sum_sq += error * error;
        if (signMismatch(dense, compressed)) {
          ++stats.sign_mismatch;
          if (std::abs(dense) <= 1.0e-2) {
            ++stats.near_surface_sign_mismatch;
          }
        }
      }
    }
  }
  if (count > 0) {
    stats.mean_abs = sum_abs / static_cast<double>(count);
    stats.rms = std::sqrt(sum_sq / static_cast<double>(count));
    stats.p95 = percentile(stats.abs_errors, 0.95);
  }
  return stats;
}

NearZeroGuardStats computeNearZeroGuardStats(
    const AdaptiveSDFBlock& reference,
    const SmallSVDResult& svd,
    double guard_band) {
  NearZeroGuardStats stats;
  for (int ix = 0; ix < reference.nx; ++ix) {
    for (int iy = 0; iy < reference.ny; ++iy) {
      for (int iz = 0; iz < reference.nz; ++iz) {
        const int col = iy * reference.nz + iz;
        const double dense =
            reference.phi[denseIndex(ix, iy, iz, reference.nx, reference.ny)];
        if (std::abs(dense) > guard_band) {
          continue;
        }
        const double compressed =
            SmallMatrixSVD::reconstructValue(svd, ix, col);
        const double abs_error = std::abs(compressed - dense);
        stats.abs_errors.push_back(abs_error);
        stats.max_abs_error = std::max(stats.max_abs_error, abs_error);
        if (signMismatch(dense, compressed)) {
          ++stats.sign_flip_count;
        }
      }
    }
  }
  stats.sample_count = stats.abs_errors.size();
  stats.p95_abs_error = percentile(stats.abs_errors, 0.95);
  return stats;
}

bool errorAcceptable(
    const ErrorStats& stats,
    const BlockLowRankCompressionOptions& options) {
  return stats.max_abs <= options.target_max_abs_error ||
         (stats.rms <= options.target_rms_error &&
          stats.p95 <= options.target_p95_error);
}

CompressedSDFBlock denseFallbackBlock(
    const AdaptiveSDFBlock& source,
    const std::string& note) {
  CompressedSDFBlock block;
  block.block_id = source.block_id;
  block.source_block_id = source.block_id;
  block.octree_node_id = source.octree_node_id;
  block.level = source.level;
  block.bounds = source.bounds;
  block.nx = source.nx;
  block.ny = source.ny;
  block.nz = source.nz;
  block.origin = source.origin;
  block.spacing = source.spacing;
  block.near_surface = source.near_surface;
  block.signed_distance = source.signed_distance;
  block.method = BlockCompressionMethod::DenseFallback;
  block.dense_phi = source.phi;
  block.max_abs_error = 0.0;
  block.mean_abs_error = 0.0;
  block.rms_error = 0.0;
  block.p95_abs_error = 0.0;
  block.original_bytes = source.phi.size() * sizeof(double);
  block.compressed_bytes = block.dense_phi.size() * sizeof(double);
  block.compression_success = true;
  block.compression_note = note;
  return block;
}

void copySVD(const SmallSVDResult& svd, MatrixSVDBlockData* target) {
  target->rows = svd.rows;
  target->cols = svd.cols;
  target->rank = svd.rank;
  target->U = svd.U;
  target->S = svd.S;
  target->Vt = svd.Vt;
}

}  // namespace

bool parseCompressionSignGuardAction(
    const std::string& value,
    CompressionSignGuardAction* action) {
  if (action == nullptr) {
    return false;
  }
  if (value == "keep-dense") {
    *action = CompressionSignGuardAction::KeepDense;
    return true;
  }
  if (value == "increase-rank") {
    *action = CompressionSignGuardAction::IncreaseRank;
    return true;
  }
  return false;
}

const char* toString(CompressionSignGuardAction action) {
  switch (action) {
    case CompressionSignGuardAction::KeepDense:
      return "keep-dense";
    case CompressionSignGuardAction::IncreaseRank:
      return "increase-rank";
  }
  return "unknown";
}

CompressedAdaptiveBlockSDF BlockLowRankCompressor::compress(
    const AdaptiveSDFBlockSet& dense_blocks,
    const BlockLowRankCompressionOptions& options,
    BlockLowRankCompressionReport* report) {
  const auto start = std::chrono::steady_clock::now();
  BlockLowRankCompressionReport local_report;
  local_report.input_block_count = dense_blocks.blocks.size();
  local_report.original_memory_bytes = dense_blocks.memoryFootprintBytes();
  local_report.compression_guard_enabled =
      options.near_zero_compression_guard;
  const std::unordered_set<int> force_dense(
      options.force_dense_block_ids.begin(),
      options.force_dense_block_ids.end());

  CompressedAdaptiveBlockSDF output;
  output.global_bounds = dense_blocks.global_bounds;
  output.signed_distance = dense_blocks.signed_distance;
  output.blocks.reserve(dense_blocks.blocks.size());

  std::vector<double> global_errors;
  std::vector<double> global_near_zero_errors;
  double global_sum_abs = 0.0;
  double global_sum_sq = 0.0;
  std::size_t global_sample_count = 0;

  if (options.method == BlockCompressionMethod::TuckerPreview) {
    local_report.warnings.push_back(
        "TuckerPreview is a planned enum only; MatrixSVD/DenseFallback output "
        "was used instead.");
  }

  for (const AdaptiveSDFBlock& source : dense_blocks.blocks) {
    if (source.near_surface) {
      ++local_report.near_surface_block_count;
    }

    if (options.always_keep_near_surface_blocks_dense && source.near_surface) {
      output.blocks.push_back(denseFallbackBlock(
          source,
          "near-surface block kept dense by option"));
      ++local_report.dense_fallback_block_count;
      continue;
    }

    if (force_dense.find(source.block_id) != force_dense.end()) {
      output.blocks.push_back(denseFallbackBlock(
          source,
          "block kept dense by block-id compression option"));
      ++local_report.dense_fallback_block_count;
      continue;
    }

    const int max_rank =
        std::max(1, std::min({options.max_rank, source.nx, source.ny * source.nz}));
    const int min_rank = std::max(1, std::min(options.min_rank, max_rank));
    int first_rank = min_rank;
    int last_rank = max_rank;
    if (options.rank_selection == RankSelectionMode::FixedRank) {
      first_rank = std::max(1, std::min(options.fixed_rank, max_rank));
      last_rank = first_rank;
    }
    if (options.rank_selection == RankSelectionMode::MemoryBounded) {
      first_rank = min_rank;
      last_rank = max_rank;
    }

    const std::vector<double> matrix = blockToMatrix(source);
    SmallSVDResult chosen_svd;
    ErrorStats chosen_stats;
    NearZeroGuardStats chosen_guard_stats;
    bool accepted = false;
    bool guard_failed_for_chosen = false;
    bool guard_sign_failed_for_chosen = false;
    bool guard_error_failed_for_chosen = false;
    std::string failure_note;

    for (int rank = first_rank; rank <= last_rank; ++rank) {
      SmallSVDOptions svd_options;
      svd_options.max_rank = rank;
      SmallSVDResult svd =
          SmallMatrixSVD::compute(matrix, source.nx, source.ny * source.nz, svd_options);
      if (!svd.success) {
        failure_note = svd.error_message;
        continue;
      }
      ErrorStats stats = computeStats(source, svd);
      const bool ok = options.rank_selection == RankSelectionMode::FixedRank ||
                      options.rank_selection == RankSelectionMode::MemoryBounded ||
                      errorAcceptable(stats, options);
      NearZeroGuardStats guard_stats;
      bool guard_ok = true;
      bool guard_sign_failed = false;
      bool guard_error_failed = false;
      if (options.near_zero_compression_guard) {
        guard_stats = computeNearZeroGuardStats(
            source,
            svd,
            options.compression_sign_guard_band);
        guard_sign_failed = guard_stats.sign_flip_count > 0;
        guard_error_failed =
            guard_stats.sample_count > 0 &&
            guard_stats.p95_abs_error >
                options.compression_near_zero_error_limit;
        guard_ok = !guard_sign_failed && !guard_error_failed;
      }
      chosen_svd = std::move(svd);
      chosen_stats = std::move(stats);
      chosen_guard_stats = std::move(guard_stats);
      guard_failed_for_chosen = !guard_ok;
      guard_sign_failed_for_chosen = guard_sign_failed;
      guard_error_failed_for_chosen = guard_error_failed;
      if (ok && guard_ok) {
        accepted = true;
        break;
      }
      if (ok && !guard_ok &&
          options.compression_sign_guard_action ==
              CompressionSignGuardAction::KeepDense) {
        failure_note = "near-zero compression guard kept block dense";
        break;
      }
    }

    if (options.near_zero_compression_guard &&
        chosen_guard_stats.sample_count > 0) {
      ++local_report.guarded_block_count;
      local_report.near_zero_compression_sign_flip_count +=
          chosen_guard_stats.sign_flip_count;
      for (double error : chosen_guard_stats.abs_errors) {
        global_near_zero_errors.push_back(error);
      }
    }

    const bool use_dense_for_guard =
        chosen_svd.success && guard_failed_for_chosen &&
        options.near_zero_compression_guard;
    if (!accepted || !chosen_svd.success || use_dense_for_guard) {
      if (use_dense_for_guard) {
        if (guard_sign_failed_for_chosen) {
          ++local_report.kept_dense_due_to_sign_count;
        }
        if (guard_error_failed_for_chosen) {
          ++local_report.kept_dense_due_to_error_count;
        }
      }
      if (options.dense_fallback_if_error_exceeds_target ||
          use_dense_for_guard) {
        output.blocks.push_back(denseFallbackBlock(
            source,
            use_dense_for_guard
                ? "dense fallback because near-zero compression guard failed"
                : (failure_note.empty()
                       ? "dense fallback because matrix-SVD did not satisfy error target"
                       : failure_note)));
        ++local_report.dense_fallback_block_count;
        continue;
      }
      local_report.warnings.push_back(
          "A block exceeded compression targets but dense fallback is disabled.");
    }

    CompressedSDFBlock block;
    block.block_id = source.block_id;
    block.source_block_id = source.block_id;
    block.octree_node_id = source.octree_node_id;
    block.level = source.level;
    block.bounds = source.bounds;
    block.nx = source.nx;
    block.ny = source.ny;
    block.nz = source.nz;
    block.origin = source.origin;
    block.spacing = source.spacing;
    block.near_surface = source.near_surface;
    block.signed_distance = source.signed_distance;
    block.method = BlockCompressionMethod::MatrixSVD;
    block.svd.nx = source.nx;
    block.svd.ny = source.ny;
    block.svd.nz = source.nz;
    copySVD(chosen_svd, &block.svd);
    block.max_abs_error = chosen_stats.max_abs;
    block.mean_abs_error = chosen_stats.mean_abs;
    block.rms_error = chosen_stats.rms;
    block.p95_abs_error = chosen_stats.p95;
    block.original_bytes = source.phi.size() * sizeof(double);
    block.compressed_bytes =
        (block.svd.U.size() + block.svd.S.size() + block.svd.Vt.size()) *
        sizeof(double);
    block.compression_success = true;
    block.compression_note = "matrix-SVD block compression";
    output.blocks.push_back(std::move(block));
    ++local_report.compressed_block_count;
    local_report.ranks_used.push_back(chosen_svd.rank);

    for (double error : chosen_stats.abs_errors) {
      global_errors.push_back(error);
      global_sum_abs += error;
      global_sum_sq += error * error;
      local_report.global_max_abs_error =
          std::max(local_report.global_max_abs_error, error);
    }
    global_sample_count += chosen_stats.abs_errors.size();
    local_report.sign_mismatch_count += chosen_stats.sign_mismatch;
    local_report.near_surface_sign_mismatch_count +=
        chosen_stats.near_surface_sign_mismatch;
  }

  local_report.compressed_memory_bytes = output.compressedMemoryBytes();
  for (const CompressedSDFBlock& block : output.blocks) {
    if (block.method == BlockCompressionMethod::DenseFallback) {
      local_report.dense_fallback_memory_bytes += block.compressed_bytes;
    }
  }
  local_report.compressed_memory_bytes_after_guard =
      local_report.compressed_memory_bytes;
  local_report.compression_ratio = output.compressionRatio();
  if (global_sample_count > 0) {
    local_report.global_mean_abs_error =
        global_sum_abs / static_cast<double>(global_sample_count);
    local_report.global_rms_error =
        std::sqrt(global_sum_sq / static_cast<double>(global_sample_count));
    local_report.global_p95_abs_error = percentile(global_errors, 0.95);
  }
  local_report.near_zero_compression_p95_error =
      percentile(global_near_zero_errors, 0.95);

  const auto end = std::chrono::steady_clock::now();
  local_report.compression_time_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  local_report.success =
      local_report.input_block_count ==
      local_report.compressed_block_count + local_report.dense_fallback_block_count;
  if (!local_report.success) {
    local_report.error_message =
        "Compressed block count does not match input block count.";
  }

  if (report) {
    *report = local_report;
  }
  return output;
}

}  // namespace adasdf
