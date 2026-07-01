#include "adasdf/recommendation/BuildSurrogateEstimator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace adasdf {
namespace {

constexpr double kBytesPerScalar = 8.0;
constexpr double kMiB = 1024.0 * 1024.0;

template <typename T>
T clampValue(T value, T lo, T hi) {
  return std::max(lo, std::min(value, hi));
}

void appendUnique(
    std::vector<std::string>& values,
    const std::string& value) {
  if (value.empty()) {
    return;
  }
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

std::string numberText(double value) {
  std::ostringstream out;
  out << value;
  return out.str();
}

int baseLevelForTarget(
    double target_error,
    const SurrogateEstimatorOptions& options) {
  int level = options.min_max_level;
  if (target_error <= 2.0e-4) {
    level = 7;
  } else if (target_error <= 1.0e-3) {
    level = 6;
  } else if (target_error <= 5.0e-3) {
    level = 5;
  } else if (target_error <= 2.0e-2) {
    level = 4;
  }
  return clampValue(level, options.min_max_level, options.max_max_level);
}

int pow2Int(int exponent) {
  if (exponent <= 0) {
    return 1;
  }
  if (exponent >= 30) {
    return 1 << 30;
  }
  return 1 << exponent;
}

double denseMemoryMb(int resolution, double safety_factor) {
  const double cells = static_cast<double>(resolution) *
                       static_cast<double>(resolution) *
                       static_cast<double>(resolution);
  return cells * kBytesPerScalar * safety_factor / kMiB;
}

double estimateSurfaceBlockCount(
    const MeshFeatureSummary& features,
    int max_level) {
  const double grid_blocks =
      std::pow(8.0, static_cast<double>(std::max(0, max_level)));
  const double tri = static_cast<double>(std::max<std::size_t>(
      1,
      features.triangle_count));
  const double surface_fraction =
      clampValue(0.035 + std::sqrt(tri) / 1600.0, 0.035, 0.28);
  const double geometric = grid_blocks * surface_fraction;
  const double triangle_cap =
      std::max(8.0, tri * static_cast<double>(max_level + 1) * 2.0 +
                        static_cast<double>(pow2Int(max_level)));
  return std::max(1.0, std::min(geometric, triangle_cap));
}

double adaptiveMemoryMb(
    const MeshFeatureSummary& features,
    int max_level,
    int block_resolution,
    double safety_factor) {
  const double block_count = estimateSurfaceBlockCount(features, max_level);
  const double cells = static_cast<double>(block_resolution) *
                       static_cast<double>(block_resolution) *
                       static_cast<double>(block_resolution);
  const double bytes = block_count * (cells * kBytesPerScalar + 160.0);
  return bytes * safety_factor / kMiB;
}

double compressedMemoryMb(
    const MeshFeatureSummary& features,
    int max_level,
    int block_resolution,
    int rank,
    bool keep_near_surface_dense,
    double safety_factor) {
  const double block_count = estimateSurfaceBlockCount(features, max_level);
  const double br = static_cast<double>(block_resolution);
  const double r = static_cast<double>(std::max(1, rank));
  const double compressed_block_bytes = (br * r + r + r * br * br) *
                                            kBytesPerScalar +
                                        192.0;
  const double dense_block_bytes = br * br * br * kBytesPerScalar + 160.0;
  const double dense_fraction = keep_near_surface_dense ? 0.18 : 0.04;
  const double bytes = block_count *
      ((1.0 - dense_fraction) * compressed_block_bytes +
       dense_fraction * dense_block_bytes);
  return bytes * safety_factor / kMiB;
}

double spatialError(
    const MeshFeatureSummary& features,
    int max_level,
    int block_resolution,
    double safety_factor) {
  const double diag =
      features.aabb_diagonal > 0.0 && std::isfinite(features.aabb_diagonal)
          ? features.aabb_diagonal
          : 1.0;
  const double denominator =
      static_cast<double>(pow2Int(max_level)) *
      static_cast<double>(std::max(1, block_resolution)) * 2.0;
  return std::max(0.0, diag / denominator) * safety_factor;
}

double denseError(
    const MeshFeatureSummary& features,
    int resolution,
    double safety_factor) {
  const double diag =
      features.aabb_diagonal > 0.0 && std::isfinite(features.aabb_diagonal)
          ? features.aabb_diagonal
          : 1.0;
  return diag / (static_cast<double>(std::max(1, resolution)) * 2.0) *
         safety_factor;
}

void addBlockResolution(std::vector<int>& values, int value) {
  if (value < 2) {
    return;
  }
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

void addDenseResolution(std::vector<int>& values, int value) {
  value = clampValue(value, 16, 384);
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

}  // namespace

BuildSurrogateEstimator::BuildSurrogateEstimator(
    const SurrogateEstimatorOptions& options)
    : options_(options) {}

const SurrogateEstimatorOptions& BuildSurrogateEstimator::options() const {
  return options_;
}

BuildEstimate BuildSurrogateEstimator::estimate(
    const MeshFeatureSummary& features,
    const BuildTarget& target,
    const BuildCandidate& candidate) const {
  BuildEstimate estimate;
  const double target_error =
      target.target_near_surface_error > 0.0
          ? target.target_near_surface_error
          : 1.0e-3;
  const double memory_budget =
      target.memory_budget_mb > 0.0 ? target.memory_budget_mb : 1.0;

  if (!features.valid) {
    appendUnique(
        estimate.warnings,
        "mesh feature summary is invalid; estimate is low confidence");
  }
  if (target.signed_distance && !features.watertight) {
    if (target.allow_open_unsigned) {
      appendUnique(
          estimate.warnings,
          "mesh is open; recommendation should use unsigned distance unless "
          "cleanup makes it watertight");
    } else {
      appendUnique(
          estimate.warnings,
          "signed distance requested for a non-watertight mesh; cleanup or "
          "unsigned mode is recommended");
    }
  }

  double raw_adaptive_memory = 0.0;
  switch (candidate.path) {
    case RecommendedBuildPath::DenseSDF:
      estimate.estimated_memory_mb = denseMemoryMb(
          std::max(1, candidate.dense_resolution),
          options_.memory_safety_factor);
      estimate.estimated_near_surface_error = denseError(
          features,
          std::max(1, candidate.dense_resolution),
          options_.error_safety_factor);
      estimate.estimated_compression_ratio = 1.0;
      estimate.estimated_build_cost_score =
          std::pow(
              static_cast<double>(std::max(1, candidate.dense_resolution)),
              3.0) /
          1.0e6;
      appendUnique(
          estimate.rationale,
          "dense memory uses resolution^3 scalar samples with a safety factor");
      break;
    case RecommendedBuildPath::AdaptiveBlockSDF:
      estimate.estimated_memory_mb = adaptiveMemoryMb(
          features,
          candidate.max_octree_level,
          candidate.block_resolution,
          options_.memory_safety_factor);
      estimate.estimated_near_surface_error = spatialError(
          features,
          candidate.max_octree_level,
          candidate.block_resolution,
          options_.error_safety_factor);
      estimate.estimated_compression_ratio = 1.0;
      estimate.estimated_build_cost_score =
          estimateSurfaceBlockCount(features, candidate.max_octree_level) *
          std::pow(static_cast<double>(candidate.block_resolution), 3.0) /
          2.0e5;
      appendUnique(
          estimate.rationale,
          "adaptive memory is estimated from surface block count and dense "
          "block payloads");
      break;
    case RecommendedBuildPath::CompressedAdaptiveBlockSDF: {
      const int rank = candidate.use_fixed_rank
                           ? candidate.fixed_rank
                           : candidate.max_rank;
      raw_adaptive_memory = adaptiveMemoryMb(
          features,
          candidate.max_octree_level,
          candidate.block_resolution,
          1.0);
      estimate.estimated_memory_mb = compressedMemoryMb(
          features,
          candidate.max_octree_level,
          candidate.block_resolution,
          rank,
          candidate.keep_near_surface_blocks_dense,
          options_.memory_safety_factor);
      const double compression_error =
          std::max(0.0, candidate.target_compression_error) *
          options_.error_safety_factor;
      estimate.estimated_near_surface_error = std::max(
          spatialError(
              features,
              candidate.max_octree_level,
              candidate.block_resolution,
              options_.error_safety_factor),
          compression_error);
      estimate.estimated_compression_ratio = std::max(
          1.0,
          raw_adaptive_memory /
              std::max(estimate.estimated_memory_mb, 1.0e-9) *
              options_.compression_ratio_safety_factor);
      estimate.estimated_build_cost_score =
          estimateSurfaceBlockCount(features, candidate.max_octree_level) *
          (std::pow(static_cast<double>(candidate.block_resolution), 3.0) /
               2.0e5 +
           static_cast<double>(std::max(1, rank)) * 0.015);
      appendUnique(
          estimate.rationale,
          "compressed memory uses a matrix-SVD block factor estimate plus "
          "dense fallback overhead");
      break;
    }
  }

  const double memory_ratio =
      estimate.estimated_memory_mb / std::max(memory_budget, 1.0e-9);
  const double error_ratio =
      estimate.estimated_near_surface_error / std::max(target_error, 1.0e-12);
  estimate.feasible = memory_ratio <= 1.0 && error_ratio <= 1.0 &&
                      features.valid;

  if (memory_ratio > 1.0) {
    appendUnique(
        estimate.warnings,
        "estimated memory exceeds the requested budget");
  }
  if (error_ratio > 1.0) {
    appendUnique(
        estimate.warnings,
        "estimated near-surface error exceeds the requested target");
  }
  if (features.readiness_score < 80) {
    appendUnique(
        estimate.warnings,
        "mesh readiness score is below 80; validate or clean before building");
  }

  double memory_weight = 22.0;
  double error_weight = 36.0;
  double cost_weight = target.prefer_fast_build ? 7.5 : 3.0;
  if (target.use_case == BuildUseCase::Contact) {
    error_weight = 45.0;
  } else if (target.use_case == BuildUseCase::Quality) {
    error_weight = 52.0;
  } else if (target.use_case == BuildUseCase::MemorySaving) {
    memory_weight = 42.0;
    error_weight = 30.0;
    cost_weight = target.prefer_fast_build ? 3.0 : 0.75;
  }

  estimate.score = memory_weight * memory_ratio +
                   error_weight * error_ratio +
                   cost_weight * estimate.estimated_build_cost_score;
  if (!estimate.feasible) {
    estimate.score += 1000.0;
  }
  if (candidate.path == RecommendedBuildPath::CompressedAdaptiveBlockSDF) {
    if (target.prefer_compression ||
        target.use_case == BuildUseCase::MemorySaving) {
      estimate.score -=
          target.use_case == BuildUseCase::MemorySaving ? 24.0 : 16.0;
    }
    if (target.use_case == BuildUseCase::Contact &&
        !candidate.keep_near_surface_blocks_dense) {
      estimate.score += 6.0;
    }
  }
  if (candidate.path == RecommendedBuildPath::DenseSDF) {
    if (!target.allow_dense_fallback) {
      estimate.score += 500.0;
      appendUnique(estimate.warnings, "dense fallback is disabled by target");
    }
    if (target.use_case == BuildUseCase::MemorySaving) {
      estimate.score += 18.0;
    }
  }
  if (target.prefer_fast_build &&
      candidate.path == RecommendedBuildPath::DenseSDF) {
    estimate.score -= 4.0;
  }

  if (estimate.feasible && features.readiness_score >= 90 &&
      memory_ratio <= 0.85 && error_ratio <= 0.85) {
    estimate.confidence = RecommendationConfidence::High;
  } else if (features.readiness_score >= 70 && memory_ratio <= 1.25 &&
             error_ratio <= 1.25) {
    estimate.confidence = RecommendationConfidence::Medium;
  } else {
    estimate.confidence = RecommendationConfidence::Low;
  }

  appendUnique(
      estimate.rationale,
      "surrogate estimate is deterministic and heuristic-calibrated, not a "
      "universal trained model");
  appendUnique(
      estimate.rationale,
      "memory ratio " + numberText(memory_ratio) + ", error ratio " +
          numberText(error_ratio));
  return estimate;
}

std::vector<BuildCandidate> BuildSurrogateEstimator::generateCandidates(
    const MeshFeatureSummary& features,
    const BuildTarget& target) const {
  std::vector<BuildCandidate> candidates;
  const double target_error =
      target.target_near_surface_error > 0.0
          ? target.target_near_surface_error
          : 1.0e-3;
  const double diag =
      features.aabb_diagonal > 0.0 && std::isfinite(features.aabb_diagonal)
          ? features.aabb_diagonal
          : 1.0;

  if (target.allow_dense_fallback) {
    std::vector<int> dense_resolutions;
    addDenseResolution(dense_resolutions, 32);
    addDenseResolution(dense_resolutions, 64);
    addDenseResolution(dense_resolutions, 96);
    addDenseResolution(dense_resolutions, 128);
    addDenseResolution(
        dense_resolutions,
        static_cast<int>(std::ceil(diag / std::max(target_error, 1.0e-9))));
    for (int resolution : dense_resolutions) {
      BuildCandidate candidate;
      candidate.path = RecommendedBuildPath::DenseSDF;
      candidate.dense_resolution = resolution;
      candidate.target_refinement_error = target_error;
      candidates.push_back(candidate);
    }
  }

  const int base_level = baseLevelForTarget(target_error, options_);
  std::vector<int> levels;
  for (int delta = -1; delta <= 1; ++delta) {
    const int level =
        clampValue(base_level + delta, options_.min_max_level, options_.max_max_level);
    if (std::find(levels.begin(), levels.end(), level) == levels.end()) {
      levels.push_back(level);
    }
  }
  if (target.use_case == BuildUseCase::Quality) {
    const int level = options_.max_max_level;
    if (std::find(levels.begin(), levels.end(), level) == levels.end()) {
      levels.push_back(level);
    }
  }

  std::vector<int> block_resolutions;
  addBlockResolution(block_resolutions, options_.min_block_resolution);
  addBlockResolution(block_resolutions, 8);
  addBlockResolution(block_resolutions, 12);
  addBlockResolution(block_resolutions, options_.max_block_resolution);
  for (int level : levels) {
    for (int block_resolution : block_resolutions) {
      if (block_resolution < options_.min_block_resolution ||
          block_resolution > options_.max_block_resolution) {
        continue;
      }
      BuildCandidate candidate;
      candidate.path = RecommendedBuildPath::AdaptiveBlockSDF;
      candidate.min_octree_level = std::max(1, level - 4);
      candidate.max_octree_level = level;
      candidate.block_resolution = block_resolution;
      candidate.target_refinement_error = target_error;
      candidates.push_back(candidate);
    }
  }

  std::vector<int> ranks;
  for (int rank : {options_.min_rank, 4, 6, 8, options_.max_rank}) {
    rank = clampValue(rank, options_.min_rank, options_.max_rank);
    if (std::find(ranks.begin(), ranks.end(), rank) == ranks.end()) {
      ranks.push_back(rank);
    }
  }
  for (int level : levels) {
    for (int block_resolution : block_resolutions) {
      if (block_resolution < options_.min_block_resolution ||
          block_resolution > options_.max_block_resolution) {
        continue;
      }
      for (int rank : ranks) {
        BuildCandidate candidate;
        candidate.path = RecommendedBuildPath::CompressedAdaptiveBlockSDF;
        candidate.min_octree_level = std::max(1, level - 4);
        candidate.max_octree_level = level;
        candidate.block_resolution = block_resolution;
        candidate.min_rank = options_.min_rank;
        candidate.max_rank = rank;
        candidate.target_refinement_error = target_error;
        candidate.target_compression_error = target_error * 0.5;
        candidate.keep_near_surface_blocks_dense =
            target.use_case == BuildUseCase::Contact;
        candidates.push_back(candidate);
      }
    }
  }

  return candidates;
}

}  // namespace adasdf
