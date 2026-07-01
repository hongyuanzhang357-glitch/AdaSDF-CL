#include "adasdf/recommendation/BuildRecommender.h"

#include <algorithm>
#include <sstream>

#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/MeshReadiness.h"
#include "adasdf/mesh/STLReader.h"
#include "adasdf/recommendation/MeshFeatureExtractor.h"
#include "adasdf/recommendation/SurrogateProfile.h"

namespace adasdf {
namespace {

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

std::string quoteArg(const std::string& value) {
  if (value.find_first_of(" \t\"") == std::string::npos) {
    return value;
  }
  std::string out = "\"";
  for (char ch : value) {
    if (ch == '"') {
      out += "\\\"";
    } else {
      out += ch;
    }
  }
  out += "\"";
  return out;
}

std::string boolSignFlag(const BuildRecipe& recipe) {
  return recipe.use_unsigned ? "--unsigned" : "--signed";
}

BuildRecipe makeRecipe(
    const BuildCandidate& candidate,
    const BuildEstimate& estimate,
    const BuildTarget& target,
    const MeshFeatureSummary& features) {
  BuildRecipe recipe;
  recipe.path = candidate.path;
  recipe.dense_resolution = candidate.dense_resolution;
  recipe.min_octree_level = candidate.min_octree_level;
  recipe.max_octree_level = candidate.max_octree_level;
  recipe.block_resolution = candidate.block_resolution;
  recipe.target_refinement_error = candidate.target_refinement_error;
  recipe.target_compression_error = candidate.target_compression_error;
  recipe.min_rank = candidate.min_rank;
  recipe.max_rank = candidate.max_rank;
  recipe.fixed_rank = candidate.fixed_rank;
  recipe.use_fixed_rank = candidate.use_fixed_rank;
  recipe.enable_low_rank =
      candidate.path == RecommendedBuildPath::CompressedAdaptiveBlockSDF;
  recipe.keep_near_surface_blocks_dense =
      candidate.keep_near_surface_blocks_dense;
  recipe.auto_clean =
      features.readiness_score < 80 ||
      features.degenerate_triangle_count > 0 ||
      features.duplicate_triangle_count > 0 ||
      features.non_manifold_edge_count > 0;
  recipe.use_unsigned =
      !target.signed_distance ||
      (target.signed_distance && !features.watertight &&
       target.allow_open_unsigned);
  recipe.signed_distance = !recipe.use_unsigned;

  recipe.estimates_feasible = estimate.feasible;
  recipe.estimated_memory_mb = estimate.estimated_memory_mb;
  recipe.estimated_near_surface_error =
      estimate.estimated_near_surface_error;
  recipe.estimated_compression_ratio =
      estimate.estimated_compression_ratio;
  recipe.estimated_build_cost_score =
      estimate.estimated_build_cost_score;
  recipe.score = estimate.score;
  recipe.confidence = estimate.confidence;
  recipe.warnings = estimate.warnings;
  recipe.rationale = estimate.rationale;

  if (target.signed_distance && !features.watertight &&
      !target.allow_open_unsigned) {
    appendUnique(
        recipe.warnings,
        "signed build requires a watertight mesh; run mesh cleanup or allow "
        "unsigned output");
    recipe.auto_clean = true;
  }
  if (recipe.use_unsigned) {
    appendUnique(
        recipe.rationale,
        "unsigned mode selected because the target or open-mesh policy allows "
        "it");
  }
  if (recipe.auto_clean) {
    appendUnique(
        recipe.rationale,
        "auto-clean is recommended because mesh readiness is below the clean "
        "build threshold");
  }
  return recipe;
}

std::string commandForRecipe(
    const BuildRecipe& recipe,
    const std::string& input_path,
    const std::string& output_basename) {
  std::ostringstream out;
  if (recipe.path == RecommendedBuildPath::DenseSDF) {
    out << "adasdf_build_dense_sdf " << quoteArg(input_path) << " "
        << quoteArg(output_basename + "_dense.sdfbin")
        << " --resolution " << recipe.dense_resolution
        << " --padding " << recipe.padding << " " << boolSignFlag(recipe)
        << " --report " << quoteArg(output_basename + "_dense_report.md");
  } else if (recipe.path == RecommendedBuildPath::AdaptiveBlockSDF) {
    out << "adasdf_build_adaptive_sdf " << quoteArg(input_path) << " "
        << quoteArg(output_basename + "_adaptive.sdfbin")
        << " --target-error " << recipe.target_refinement_error
        << " --min-level " << recipe.min_octree_level
        << " --max-level " << recipe.max_octree_level
        << " --block-resolution " << recipe.block_resolution
        << " --padding " << recipe.padding << " " << boolSignFlag(recipe)
        << " --report " << quoteArg(output_basename + "_adaptive_report.md");
  } else {
    out << "adasdf_build_compressed_sdf " << quoteArg(input_path) << " "
        << quoteArg(output_basename + "_compressed.sdfbin")
        << " --target-error " << recipe.target_refinement_error
        << " --min-level " << recipe.min_octree_level
        << " --max-level " << recipe.max_octree_level
        << " --block-resolution " << recipe.block_resolution
        << " --padding " << recipe.padding << " " << boolSignFlag(recipe)
        << " --max-rank " << recipe.max_rank
        << " --report " << quoteArg(output_basename + "_build_report.md")
        << " --compression-report "
        << quoteArg(output_basename + "_compression_report.md");
    if (recipe.use_fixed_rank && recipe.fixed_rank > 0) {
      out << " --fixed-rank " << recipe.fixed_rank;
    }
    if (recipe.keep_near_surface_blocks_dense) {
      out << " --keep-near-surface-dense";
    }
  }
  if (recipe.auto_clean) {
    out << " --auto-clean";
  }
  return out.str();
}

bool pathEnabled(
    const BuildRecommenderOptions& options,
    RecommendedBuildPath path) {
  if (path == RecommendedBuildPath::DenseSDF) {
    return options.include_dense;
  }
  if (path == RecommendedBuildPath::AdaptiveBlockSDF) {
    return options.include_adaptive;
  }
  return options.include_compressed;
}

std::vector<std::string> limitations() {
  return {
      "v1.8.0-alpha recommendation is experimental and deterministic.",
      "It is not a universal trained model.",
      "It is not fully trained.",
      "It is not an optimality guarantee.",
      "It does not run the build by default.",
      "Validate the final SDF with build, compression, and quality reports.",
      "Full low-rank compressed SDF GPU-native query remains planned work."};
}

}  // namespace

BuildRecommender::BuildRecommender(
    const BuildRecommenderOptions& options)
    : options_(options) {}

BuildRecommendationReport BuildRecommender::recommendFromMesh(
    const TriangleMesh& mesh,
    const BuildTarget& target,
    const std::string& input_path_for_command,
    const std::string& output_basename) const {
  MeshDiagnosticsReport diagnostics = MeshDiagnostics::analyze(mesh);
  MeshReadinessOptions readiness_options;
  readiness_options.require_watertight = target.signed_distance;
  if (target.allow_open_unsigned) {
    readiness_options.require_watertight = false;
  }
  MeshReadinessReport readiness =
      MeshReadiness::evaluate(diagnostics, readiness_options);
  MeshFeatureSummary features =
      MeshFeatureExtractor::fromMesh(mesh, diagnostics, readiness);

  BuildRecommendationReport report;
  report.target = target;
  report.mesh_features = features;
  report.limitations = limitations();
  for (const std::string& warning : features.warnings) {
    appendUnique(report.global_warnings, warning);
  }
  if (target.signed_distance && !features.watertight) {
    appendUnique(
        report.global_warnings,
        "signed distance targets need watertight input; this recommendation "
        "may require cleanup or unsigned mode");
  }

  if (!features.valid) {
    report.success = false;
    report.error_message =
        "mesh is not valid enough for build recommendation";
    report.summary =
        "No reliable build recommendation was produced for this mesh.";
    return report;
  }

  BuildSurrogateEstimator estimator(options_.estimator_options);
  std::vector<BuildCandidate> candidates =
      estimator.generateCandidates(features, target);
  std::vector<BuildRecipe> recipes;
  recipes.reserve(candidates.size());
  for (const BuildCandidate& candidate : candidates) {
    if (!pathEnabled(options_, candidate.path)) {
      continue;
    }
    BuildRecipe recipe =
        makeRecipe(candidate, estimator.estimate(features, target, candidate),
                   target, features);
    if (options_.emit_cli_command) {
      recipe.cli_command =
          commandForRecipe(recipe, input_path_for_command, output_basename);
    }
    recipes.push_back(recipe);
  }
  std::stable_sort(
      recipes.begin(),
      recipes.end(),
      [](const BuildRecipe& a, const BuildRecipe& b) {
        if (a.estimates_feasible != b.estimates_feasible) {
          return a.estimates_feasible;
        }
        return a.score < b.score;
      });
  if (options_.max_candidates > 0 &&
      static_cast<int>(recipes.size()) > options_.max_candidates) {
    recipes.resize(static_cast<std::size_t>(options_.max_candidates));
  }

  report.candidate_recipes = recipes;
  if (recipes.empty()) {
    report.success = false;
    report.error_message = "no build candidates were enabled";
    report.summary =
        "No recommendation was produced because all candidate families were "
        "disabled.";
    return report;
  }
  report.recommended_recipe = recipes.front();
  report.success = true;
  report.summary =
      "AdaSDF-CL build recommender selected " +
      std::string(toString(report.recommended_recipe.path)) +
      " using a deterministic surrogate-guided estimate.";
  if (!report.recommended_recipe.estimates_feasible) {
    appendUnique(
        report.global_warnings,
        "best candidate does not fully satisfy the requested memory/error "
        "target; inspect candidate table before building");
  }
  return report;
}

BuildRecommendationReport BuildRecommender::recommendFromSTL(
    const std::string& stl_path,
    const BuildTarget& target,
    const std::string& output_basename) const {
  MeshDiagnosticsReport diagnostics;
  MeshReadinessReport readiness;
  MeshFeatureSummary features =
      MeshFeatureExtractor::fromSTL(stl_path, &diagnostics, &readiness);
  if (!features.valid) {
    BuildRecommendationReport report;
    report.success = false;
    report.target = target;
    report.mesh_features = features;
    report.error_message =
        features.warnings.empty() ? "failed to extract mesh features"
                                  : features.warnings.front();
    report.global_warnings = features.warnings;
    report.limitations = limitations();
    report.summary =
        "No reliable build recommendation was produced for this STL.";
    return report;
  }

  const STLReadResult read = STLReader::read(stl_path);
  if (!read.success) {
    BuildRecommendationReport report;
    report.success = false;
    report.target = target;
    report.mesh_features = features;
    report.error_message = "failed to reread STL: " + read.error_message;
    report.limitations = limitations();
    return report;
  }
  return recommendFromMesh(
      read.mesh,
      target,
      stl_path,
      output_basename);
}

}  // namespace adasdf
