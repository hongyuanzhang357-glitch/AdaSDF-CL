#include "adasdf/recommendation/BuildRecommendationWriter.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace adasdf {
namespace {

std::string yesNo(bool value) {
  return value ? "yes" : "no";
}

std::string jsonEscape(const std::string& value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += ch;
        break;
    }
  }
  return out;
}

void writeStringArray(
    std::ostringstream& out,
    const std::vector<std::string>& values,
    int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << "[";
  if (!values.empty()) {
    out << "\n";
    for (std::size_t i = 0; i < values.size(); ++i) {
      out << pad << "  \"" << jsonEscape(values[i]) << "\"";
      if (i + 1 != values.size()) {
        out << ",";
      }
      out << "\n";
    }
    out << pad;
  }
  out << "]";
}

void writeRecipeJson(
    std::ostringstream& out,
    const BuildRecipe& recipe,
    int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << "{\n";
  out << pad << "  \"path\": \"" << toString(recipe.path) << "\",\n";
  out << pad << "  \"dense_resolution\": " << recipe.dense_resolution << ",\n";
  out << pad << "  \"min_octree_level\": " << recipe.min_octree_level << ",\n";
  out << pad << "  \"max_octree_level\": " << recipe.max_octree_level << ",\n";
  out << pad << "  \"block_resolution\": " << recipe.block_resolution << ",\n";
  out << pad << "  \"target_refinement_error\": "
      << recipe.target_refinement_error << ",\n";
  out << pad << "  \"target_compression_error\": "
      << recipe.target_compression_error << ",\n";
  out << pad << "  \"min_rank\": " << recipe.min_rank << ",\n";
  out << pad << "  \"max_rank\": " << recipe.max_rank << ",\n";
  out << pad << "  \"fixed_rank\": " << recipe.fixed_rank << ",\n";
  out << pad << "  \"use_fixed_rank\": "
      << (recipe.use_fixed_rank ? "true" : "false") << ",\n";
  out << pad << "  \"enable_low_rank\": "
      << (recipe.enable_low_rank ? "true" : "false") << ",\n";
  out << pad << "  \"keep_near_surface_blocks_dense\": "
      << (recipe.keep_near_surface_blocks_dense ? "true" : "false") << ",\n";
  out << pad << "  \"auto_clean\": "
      << (recipe.auto_clean ? "true" : "false") << ",\n";
  out << pad << "  \"signed_distance\": "
      << (recipe.signed_distance ? "true" : "false") << ",\n";
  out << pad << "  \"estimates_feasible\": "
      << (recipe.estimates_feasible ? "true" : "false") << ",\n";
  out << pad << "  \"estimated_memory_mb\": "
      << recipe.estimated_memory_mb << ",\n";
  out << pad << "  \"estimated_near_surface_error\": "
      << recipe.estimated_near_surface_error << ",\n";
  out << pad << "  \"estimated_compression_ratio\": "
      << recipe.estimated_compression_ratio << ",\n";
  out << pad << "  \"estimated_build_cost_score\": "
      << recipe.estimated_build_cost_score << ",\n";
  out << pad << "  \"confidence\": \"" << toString(recipe.confidence)
      << "\",\n";
  out << pad << "  \"cli_command\": \""
      << jsonEscape(recipe.cli_command) << "\",\n";
  out << pad << "  \"warnings\": ";
  writeStringArray(out, recipe.warnings, indent + 2);
  out << ",\n" << pad << "  \"rationale\": ";
  writeStringArray(out, recipe.rationale, indent + 2);
  out << "\n" << pad << "}";
}

}  // namespace

std::string BuildRecommendationWriter::toMarkdown(
    const BuildRecommendationReport& report) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "# AdaSDF-CL Build Recommendation\n\n";
  out << report.summary << "\n\n";
  out << "## Target\n\n";
  out << "- Target near-surface error: "
      << report.target.target_near_surface_error << "\n";
  out << "- Memory budget MB: " << report.target.memory_budget_mb << "\n";
  out << "- Use case: " << toString(report.target.use_case) << "\n";
  out << "- Signed distance: " << yesNo(report.target.signed_distance) << "\n";
  out << "- Allow open unsigned: "
      << yesNo(report.target.allow_open_unsigned) << "\n";
  out << "- Allow dense fallback: "
      << yesNo(report.target.allow_dense_fallback) << "\n";
  out << "- Prefer compression: "
      << yesNo(report.target.prefer_compression) << "\n\n";

  out << "## Mesh Features\n\n";
  out << "- Valid: " << yesNo(report.mesh_features.valid) << "\n";
  out << "- Vertices: " << report.mesh_features.vertex_count << "\n";
  out << "- Triangles: " << report.mesh_features.triangle_count << "\n";
  out << "- AABB diagonal: " << report.mesh_features.aabb_diagonal << "\n";
  out << "- AABB volume: " << report.mesh_features.aabb_volume << "\n";
  out << "- Watertight: " << yesNo(report.mesh_features.watertight) << "\n";
  out << "- Boundary edges: " << report.mesh_features.boundary_edge_count << "\n";
  out << "- Non-manifold edges: "
      << report.mesh_features.non_manifold_edge_count << "\n";
  out << "- Degenerate triangles: "
      << report.mesh_features.degenerate_triangle_count << "\n";
  out << "- Duplicate triangles: "
      << report.mesh_features.duplicate_triangle_count << "\n";
  out << "- Components: "
      << report.mesh_features.connected_component_count << "\n";
  out << "- Readiness score: " << report.mesh_features.readiness_score << "\n";
  out << "- Readiness level: " << report.mesh_features.readiness_level
      << "\n\n";

  out << "## Recommended Path\n\n";
  const BuildRecipe& recipe = report.recommended_recipe;
  out << "- Recommended path: " << toString(recipe.path) << "\n";
  out << "- Confidence: " << toString(recipe.confidence) << "\n";
  out << "- Feasible estimate: " << yesNo(recipe.estimates_feasible) << "\n";
  out << "- Estimated memory MB: " << recipe.estimated_memory_mb << "\n";
  out << "- Estimated near-surface error: "
      << recipe.estimated_near_surface_error << "\n";
  out << "- Estimated compression ratio: "
      << recipe.estimated_compression_ratio << "\n";
  out << "- Estimated build cost score: "
      << recipe.estimated_build_cost_score << "\n";
  out << "- Dense resolution: " << recipe.dense_resolution << "\n";
  out << "- Octree levels: " << recipe.min_octree_level << "-"
      << recipe.max_octree_level << "\n";
  out << "- Block resolution: " << recipe.block_resolution << "\n";
  out << "- Rank range: " << recipe.min_rank << "-" << recipe.max_rank
      << "\n";
  out << "- Keep near-surface blocks dense: "
      << yesNo(recipe.keep_near_surface_blocks_dense) << "\n";
  out << "- Auto-clean: " << yesNo(recipe.auto_clean) << "\n\n";

  out << "## CLI command\n\n";
  out << "```bash\n" << recipe.cli_command << "\n```\n\n";

  out << "## Rationale\n\n";
  for (const std::string& line : recipe.rationale) {
    out << "- " << line << "\n";
  }
  out << "\n## Warnings\n\n";
  if (report.global_warnings.empty() && recipe.warnings.empty()) {
    out << "- none\n";
  } else {
    for (const std::string& line : report.global_warnings) {
      out << "- " << line << "\n";
    }
    for (const std::string& line : recipe.warnings) {
      out << "- " << line << "\n";
    }
  }

  out << "\n## Candidate Table\n\n";
  out << "| Path | Feasible | Confidence | Memory MB | Error | Compression | Cost | Command |\n";
  out << "| --- | --- | --- | ---: | ---: | ---: | ---: | --- |\n";
  for (const BuildRecipe& candidate : report.candidate_recipes) {
    out << "| " << toString(candidate.path)
        << " | " << yesNo(candidate.estimates_feasible)
        << " | " << toString(candidate.confidence)
        << " | " << candidate.estimated_memory_mb
        << " | " << candidate.estimated_near_surface_error
        << " | " << candidate.estimated_compression_ratio
        << " | " << candidate.estimated_build_cost_score
        << " | `" << candidate.cli_command << "` |\n";
  }

  out << "\n## Recommended Validation Commands\n\n";
  out << "- Run the emitted build command.\n";
  out << "- Inspect the generated build report.\n";
  out << "- For compressed output, inspect the compression and quality reports.\n";
  out << "- Use `adasdf_info` and `adasdf_query` on the generated `.sdfbin`.\n\n";

  out << "## Limitations\n\n";
  for (const std::string& limitation : report.limitations) {
    out << "- " << limitation << "\n";
  }
  return out.str();
}

std::string BuildRecommendationWriter::toJson(
    const BuildRecommendationReport& report) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "{\n";
  out << "  \"success\": " << (report.success ? "true" : "false") << ",\n";
  out << "  \"error_message\": \""
      << jsonEscape(report.error_message) << "\",\n";
  out << "  \"summary\": \"" << jsonEscape(report.summary) << "\",\n";
  out << "  \"target\": {\n";
  out << "    \"target_near_surface_error\": "
      << report.target.target_near_surface_error << ",\n";
  out << "    \"memory_budget_mb\": " << report.target.memory_budget_mb
      << ",\n";
  out << "    \"use_case\": \"" << toString(report.target.use_case)
      << "\",\n";
  out << "    \"signed_distance\": "
      << (report.target.signed_distance ? "true" : "false") << ",\n";
  out << "    \"allow_open_unsigned\": "
      << (report.target.allow_open_unsigned ? "true" : "false") << ",\n";
  out << "    \"allow_dense_fallback\": "
      << (report.target.allow_dense_fallback ? "true" : "false") << ",\n";
  out << "    \"prefer_compression\": "
      << (report.target.prefer_compression ? "true" : "false") << "\n";
  out << "  },\n";
  out << "  \"mesh_features\": {\n";
  out << "    \"valid\": "
      << (report.mesh_features.valid ? "true" : "false") << ",\n";
  out << "    \"vertex_count\": " << report.mesh_features.vertex_count << ",\n";
  out << "    \"triangle_count\": " << report.mesh_features.triangle_count
      << ",\n";
  out << "    \"aabb_diagonal\": " << report.mesh_features.aabb_diagonal
      << ",\n";
  out << "    \"aabb_volume\": " << report.mesh_features.aabb_volume << ",\n";
  out << "    \"watertight\": "
      << (report.mesh_features.watertight ? "true" : "false") << ",\n";
  out << "    \"readiness_score\": "
      << report.mesh_features.readiness_score << "\n";
  out << "  },\n";
  out << "  \"recommended_recipe\": ";
  writeRecipeJson(out, report.recommended_recipe, 2);
  out << ",\n";
  out << "  \"candidate_recipes\": [\n";
  for (std::size_t i = 0; i < report.candidate_recipes.size(); ++i) {
    out << "    ";
    writeRecipeJson(out, report.candidate_recipes[i], 4);
    if (i + 1 != report.candidate_recipes.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
  out << "  \"global_warnings\": ";
  writeStringArray(out, report.global_warnings, 2);
  out << ",\n";
  out << "  \"limitations\": ";
  writeStringArray(out, report.limitations, 2);
  out << "\n}\n";
  return out.str();
}

void BuildRecommendationWriter::writeMarkdown(
    const std::string& path,
    const BuildRecommendationReport& report) {
  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open recommendation markdown: " + path);
  }
  file << toMarkdown(report);
}

void BuildRecommendationWriter::writeJson(
    const std::string& path,
    const BuildRecommendationReport& report) {
  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open recommendation json: " + path);
  }
  file << toJson(report);
}

}  // namespace adasdf
