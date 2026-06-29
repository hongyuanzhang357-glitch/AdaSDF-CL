#include "adasdf/generation/DemoSurrogateRecommender.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace adasdf {
namespace {

double clampDouble(double value, double lo, double hi) {
  return std::max(lo, std::min(value, hi));
}

int clampInt(int value, int lo, int hi) {
  return std::max(lo, std::min(value, hi));
}

int baseLevelForError(double error) {
  if (error <= 1.0e-5) {
    return 7;
  }
  if (error <= 1.0e-4) {
    return 6;
  }
  if (error <= 1.0e-3) {
    return 5;
  }
  if (error <= 1.0e-2) {
    return 4;
  }
  return 3;
}

int baseResolutionForError(double error) {
  if (error <= 1.0e-5) {
    return 64;
  }
  if (error <= 1.0e-4) {
    return 48;
  }
  if (error <= 1.0e-3) {
    return 32;
  }
  if (error <= 1.0e-2) {
    return 24;
  }
  return 16;
}

int baseRankForError(double error) {
  if (error <= 1.0e-5) {
    return 14;
  }
  if (error <= 1.0e-4) {
    return 12;
  }
  if (error <= 1.0e-3) {
    return 8;
  }
  if (error <= 1.0e-2) {
    return 6;
  }
  return 4;
}

}  // namespace

const char* DemoSurrogateRecommender::id() {
  return "demo_v0_9";
}

const char* DemoSurrogateRecommender::statusWarning() {
  return "experimental demo surrogate; not universal; not fully trained; "
         "not an optimality guarantee";
}

std::vector<DemoSurrogateCandidate> DemoSurrogateRecommender::recommend(
    const DemoSurrogateInput& input) {
  if (input.shape_type != "box") {
    throw std::runtime_error(
        "DemoSurrogateRecommender currently supports shape_type=box only.");
  }
  if (!(input.target_near_surface_error > 0.0) ||
      !(input.memory_limit_mb > 0.0) ||
      !(input.block_expand_limit_mb > 0.0)) {
    throw std::runtime_error(
        "DemoSurrogateRecommender inputs must be positive.");
  }

  const int top_k = clampInt(input.top_k, 1, 16);
  const double memory_scale = clampDouble(input.memory_limit_mb / 64.0, 0.25, 4.0);
  const double block_scale =
      clampDouble(input.block_expand_limit_mb / 16.0, 0.25, 4.0);

  const int base_level = baseLevelForError(input.target_near_surface_error);
  const int base_resolution = baseResolutionForError(input.target_near_surface_error);
  const int base_rank = baseRankForError(input.target_near_surface_error);

  std::vector<DemoSurrogateCandidate> candidates;
  candidates.reserve(static_cast<std::size_t>(top_k));
  for (int i = 0; i < top_k; ++i) {
    const int exploration = i - (top_k / 2);
    const int memory_penalty = input.memory_limit_mb < 24.0 ? 2 :
        (input.memory_limit_mb < 48.0 ? 1 : 0);
    const int block_penalty = input.block_expand_limit_mb < 8.0 ? 2 :
        (input.block_expand_limit_mb < 16.0 ? 1 : 0);

    const int level = clampInt(base_level + exploration - memory_penalty, 2, 8);
    const int resolution = clampInt(
        base_resolution + exploration * 8 - block_penalty * 8, 8, 64);
    const int rank = clampInt(
        static_cast<int>(std::round(base_rank * std::sqrt(memory_scale))) +
            exploration - block_penalty,
        2,
        32);

    BuildOptions options;
    options.near_surface_error = input.target_near_surface_error;
    options.tau_near_abs = input.target_near_surface_error;
    options.max_memory_mb =
        static_cast<std::size_t>(std::ceil(input.memory_limit_mb));
    options.memory_limit_mb = input.memory_limit_mb;
    options.block_expand_limit_mb =
        static_cast<std::size_t>(std::ceil(input.block_expand_limit_mb));
    options.block_memory_limit_mb = input.block_expand_limit_mb;
    options.max_octree_level = level;
    options.min_octree_level = std::max(1, level - 3);
    options.base_block_cells = resolution;
    options.min_block_cells = std::max(8, resolution / 2);
    options.max_rank = rank;
    options.min_rank = std::max(2, rank / 3);
    options.near_min_rank = std::max(options.min_rank, rank / 2);
    options.compression_method = "demo_low_rank";
    options.enable_compression = true;
    options.enable_adaptive_blocks = true;
    options.use_surrogate_recommendation = true;
    options.surrogate_top_k = top_k;
    options.query_mode = QueryMode::Balanced;

    const double complexity = static_cast<double>(level * resolution * rank);
    const double predicted_memory =
        clampDouble(complexity / 2600.0, 0.25, input.memory_limit_mb);
    const double predicted_error =
        input.target_near_surface_error * (1.0 + 0.12 * std::abs(exploration));
    const double confidence = clampDouble(
        0.76 - 0.04 * std::abs(exploration) +
            0.04 * std::min(memory_scale, block_scale),
        0.35,
        0.88);

    DemoSurrogateCandidate candidate;
    candidate.options = options;
    candidate.predicted_p95_error = predicted_error;
    candidate.predicted_memory_mb = predicted_memory;
    candidate.confidence = confidence;
    candidate.credible_demo_only = true;
    candidate.warning = statusWarning();
    candidates.push_back(candidate);
  }

  std::stable_sort(
      candidates.begin(),
      candidates.end(),
      [](const DemoSurrogateCandidate& a, const DemoSurrogateCandidate& b) {
        if (a.predicted_p95_error != b.predicted_p95_error) {
          return a.predicted_p95_error < b.predicted_p95_error;
        }
        return a.predicted_memory_mb < b.predicted_memory_mb;
      });
  return candidates;
}

}  // namespace adasdf
