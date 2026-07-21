#pragma once

#include <string>
#include <vector>

namespace adasdf {

enum class NarrowBandTensorFillMode {
  ExactAll,
  ContactExactFarInterp,
  CoarseProlongation
};

enum class NarrowBandCompressionMode {
  WeightedLowRank,
  Dense,
  ExistingLowRank
};

struct BrickLevelMapRange {
  int sampling_level_min = 0;
  int sampling_level_max = 0;
  int brick_level = 0;
};

struct NarrowBandBrickBuildOptions {
  int max_sampling_level = 5;
  std::vector<BrickLevelMapRange> brick_level_map;

  int brick_min_tensor_dim = 12;
  int brick_target_tensor_dim = 24;
  int brick_max_tensor_dim = 32;
  int brick_max_expanded_kb = 512;
  int brick_min_compression_nodes = 64;
  bool brick_split_on_high_rank = false;
  bool brick_split_on_curvature = false;
  bool brick_split_on_memory = false;
  bool brick_merge_small = false;
  int sampling_levels_per_brick_level = 0;

  double sampling_contact_band_width = 0.01;
  bool sampling_curvature_aware = false;
  bool sampling_small_gap_aware = false;
  bool sampling_refine_zero_crossing = true;
  bool sampling_refine_contact_band = true;
  bool sampling_refine_curvature_hint = false;
  bool sampling_refine_small_gap_hint = false;
  std::string sampling_mode = "collision";

  NarrowBandTensorFillMode tensor_fill =
      NarrowBandTensorFillMode::ContactExactFarInterp;
  NarrowBandCompressionMode compression_mode =
      NarrowBandCompressionMode::WeightedLowRank;
  double contact_weight = 10.0;
  double far_field_weight = 1.0;
  bool near_zero_sign_guard = true;
  bool normal_guard = false;
  bool dense_fallback_on_sign_flip = true;
  bool dense_fallback_on_contact_error = false;
  int max_rank = 8;
  bool rank_auto = false;

  double contact_exact_spacing = 0.0;
  double contact_exact_min_node_ratio = 0.0;
  int contact_exact_stencil = 1;
  int zero_crossing_exact_stencil = 1;
  int contact_exact_from_surface_samples = 0;
  bool contact_exact_from_zero_crossing_cells = false;
  std::vector<double> contact_exact_normal_offsets;

  bool sign_protected_fill = false;
  std::string zero_crossing_cell_fill = "linear";
  bool fill_sign_check = false;
  std::string fill_sign_fallback = "linear";
  std::string query_backend = "auto";

  int thread_count = 0;
  double max_seconds = 0.0;
};

bool parseBrickLevelMap(
    const std::string& text,
    std::vector<BrickLevelMapRange>* ranges,
    std::string* error = nullptr);

int brickLevelForSamplingLevel(
    const std::vector<BrickLevelMapRange>& ranges,
    int sampling_level);

const char* toString(NarrowBandTensorFillMode mode);
bool parseNarrowBandTensorFillMode(
    const std::string& text,
    NarrowBandTensorFillMode* mode);

const char* toString(NarrowBandCompressionMode mode);
bool parseNarrowBandCompressionMode(
    const std::string& text,
    NarrowBandCompressionMode* mode);

}  // namespace adasdf
