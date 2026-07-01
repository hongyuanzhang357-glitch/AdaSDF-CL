#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace adasdf {

enum class BuildUseCase {
  Contact,
  Balanced,
  Quality,
  MemorySaving
};

enum class RecommendedBuildPath {
  DenseSDF,
  AdaptiveBlockSDF,
  CompressedAdaptiveBlockSDF
};

enum class RecommendationConfidence {
  Low,
  Medium,
  High
};

struct BuildTarget {
  double target_near_surface_error = 1.0e-3;
  double memory_budget_mb = 512.0;
  BuildUseCase use_case = BuildUseCase::Contact;
  bool signed_distance = true;
  bool allow_open_unsigned = false;
  bool allow_dense_fallback = true;
  bool prefer_compression = true;
  bool prefer_fast_build = false;
  double max_build_time_hint_seconds = 0.0;
};

struct MeshFeatureSummary {
  bool valid = false;

  std::size_t vertex_count = 0;
  std::size_t triangle_count = 0;
  double aabb_diagonal = 0.0;
  double aabb_volume = 0.0;

  bool watertight = false;
  bool likely_oriented = false;
  bool has_nan_or_inf = false;
  bool has_degenerate_aabb = false;

  std::size_t boundary_edge_count = 0;
  std::size_t non_manifold_edge_count = 0;
  std::size_t degenerate_triangle_count = 0;
  std::size_t duplicate_triangle_count = 0;
  std::size_t connected_component_count = 0;

  int readiness_score = 0;
  std::string readiness_level;
  std::vector<std::string> warnings;
};

struct BuildRecipe {
  RecommendedBuildPath path = RecommendedBuildPath::AdaptiveBlockSDF;

  int dense_resolution = 0;
  int min_octree_level = 1;
  int max_octree_level = 5;
  int block_resolution = 8;
  double padding = 0.05;

  double target_refinement_error = 1.0e-3;
  double target_compression_error = 1.0e-3;

  int min_rank = 1;
  int max_rank = 8;
  int fixed_rank = 0;
  bool use_fixed_rank = false;
  bool enable_low_rank = false;
  bool keep_near_surface_blocks_dense = false;
  bool auto_clean = false;
  bool signed_distance = true;
  bool use_unsigned = false;

  bool estimates_feasible = false;
  double estimated_memory_mb = 0.0;
  double estimated_near_surface_error = 0.0;
  double estimated_compression_ratio = 1.0;
  double estimated_build_cost_score = 0.0;
  double score = 0.0;
  RecommendationConfidence confidence = RecommendationConfidence::Low;

  std::vector<std::string> warnings;
  std::vector<std::string> rationale;
  std::string cli_command;
};

struct BuildRecommendationReport {
  bool success = false;
  std::string error_message;

  BuildTarget target;
  MeshFeatureSummary mesh_features;
  BuildRecipe recommended_recipe;
  std::vector<BuildRecipe> candidate_recipes;

  std::vector<std::string> global_warnings;
  std::vector<std::string> limitations;
  std::string summary;
};

inline std::string normalizeBuildToken(std::string value) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char ch) {
        if (ch == '_' || ch == ' ') {
          return '-';
        }
        return static_cast<char>(std::tolower(ch));
      });
  return value;
}

inline const char* toString(BuildUseCase use_case) {
  switch (use_case) {
    case BuildUseCase::Contact:
      return "contact";
    case BuildUseCase::Balanced:
      return "balanced";
    case BuildUseCase::Quality:
      return "quality";
    case BuildUseCase::MemorySaving:
      return "memory-saving";
  }
  return "balanced";
}

inline bool parseBuildUseCase(
    const std::string& value,
    BuildUseCase* use_case) {
  if (!use_case) {
    return false;
  }
  const std::string normalized = normalizeBuildToken(value);
  if (normalized == "contact") {
    *use_case = BuildUseCase::Contact;
    return true;
  }
  if (normalized == "balanced" || normalized == "default") {
    *use_case = BuildUseCase::Balanced;
    return true;
  }
  if (normalized == "quality" || normalized == "accuracy") {
    *use_case = BuildUseCase::Quality;
    return true;
  }
  if (normalized == "memory-saving" || normalized == "memory" ||
      normalized == "compressed" || normalized == "compression") {
    *use_case = BuildUseCase::MemorySaving;
    return true;
  }
  return false;
}

inline const char* toString(RecommendedBuildPath path) {
  switch (path) {
    case RecommendedBuildPath::DenseSDF:
      return "DenseSDF";
    case RecommendedBuildPath::AdaptiveBlockSDF:
      return "AdaptiveBlockSDF";
    case RecommendedBuildPath::CompressedAdaptiveBlockSDF:
      return "CompressedAdaptiveBlockSDF";
  }
  return "AdaptiveBlockSDF";
}

inline bool parseRecommendedBuildPath(
    const std::string& value,
    RecommendedBuildPath* path) {
  if (!path) {
    return false;
  }
  const std::string normalized = normalizeBuildToken(value);
  if (normalized == "dense" || normalized == "densesdf") {
    *path = RecommendedBuildPath::DenseSDF;
    return true;
  }
  if (normalized == "adaptive" || normalized == "adaptive-block" ||
      normalized == "adaptiveblocksdf") {
    *path = RecommendedBuildPath::AdaptiveBlockSDF;
    return true;
  }
  if (normalized == "compressed" ||
      normalized == "compressed-adaptive-block" ||
      normalized == "compressedadaptiveblocksdf") {
    *path = RecommendedBuildPath::CompressedAdaptiveBlockSDF;
    return true;
  }
  return false;
}

inline const char* toString(RecommendationConfidence confidence) {
  switch (confidence) {
    case RecommendationConfidence::Low:
      return "low";
    case RecommendationConfidence::Medium:
      return "medium";
    case RecommendationConfidence::High:
      return "high";
  }
  return "low";
}

}  // namespace adasdf
