#pragma once

#include <cstddef>
#include <string>

#include "adasdf/backend/Backend.h"
#include "adasdf/compression/CompressionOptions.h"

namespace adasdf {

struct BuildOptions {
  // User-facing accuracy targets.
  Scalar near_surface_error = 1e-4;
  Scalar far_field_error = 1e-3;

  // Memory constraints.
  std::size_t max_memory_mb = 512;
  std::size_t block_expand_limit_mb = 128;

  // Adaptive construction.
  bool enable_compression = true;
  bool enable_adaptive_blocks = true;
  bool enable_contact_only_export = false;

  int max_octree_level = 8;
  int min_octree_level = 2;
  int max_rank = 32;

  // Compatibility with DASH-SDF surrogate recommendation terminology.
  Scalar tau_near_abs = 1e-4;
  Scalar memory_limit_mb = 512.0;
  Scalar block_memory_limit_mb = 128.0;

  BackendType backend = BackendType::CPU;
  QueryMode query_mode = QueryMode::Balanced;

  std::string unit = "m";
  std::string compression_method = "auto";
  std::string output_format = "sdfbin";

  bool use_surrogate_recommendation = false;
  int surrogate_top_k = 5;

  bool verbose = true;

  // Existing-core bridge parameters kept compact and intentionally non-exhaustive.
  Scalar box_margin_rate = 0.05;
  int bvh_leaf_size = 16;
  int sign_ray_count = 3;
  int surface_band_cells = 2;
  int base_block_cells = 32;
  int min_block_cells = 16;
  int ghost_cells = 2;
  int min_rank = 2;
  int near_min_rank = 4;
  int rank_step = 2;
  Scalar rank_energy_percent = 0.0;
  Scalar near_band_over_h = 3.0;
  CompressionOptions compression;
};

}  // namespace adasdf
