#include "adasdf/audit/MismatchCellReportWriter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace adasdf {
namespace {

std::string boolText(bool value) {
  return value ? "true" : "false";
}

std::string jsonEscape(const std::string& text) {
  std::ostringstream out;
  for (char c : text) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << c;
        break;
    }
  }
  return out.str();
}

std::string csvEscape(const std::string& text) {
  if (text.find_first_of(",\"\n\r") == std::string::npos) {
    return text;
  }
  std::string out = "\"";
  for (char c : text) {
    if (c == '"') {
      out += "\"\"";
    } else {
      out += c;
    }
  }
  out += "\"";
  return out;
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string joinDoubles(const std::vector<double>& values) {
  std::ostringstream out;
  out << std::setprecision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ";";
    }
    out << values[i];
  }
  return out.str();
}

std::string joinInts(const std::vector<int>& values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ";";
    }
    out << values[i];
  }
  return out.str();
}

std::string subgridJson(
    const std::vector<MismatchCellSubgridSummary>& summaries) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < summaries.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"subgrid\":" << summaries[i].subgrid
        << ",\"fixed_count\":" << summaries[i].fixed_count
        << ",\"remaining_mismatch_count\":"
        << summaries[i].remaining_mismatch_count << "}";
  }
  out << "]";
  return out.str();
}

std::string statsJson(const std::vector<MismatchCellPatternStats>& stats) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "[";
  for (std::size_t i = 0; i < stats.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    const MismatchCellPatternStats& s = stats[i];
    out << "{\"key\":\"" << jsonEscape(s.key) << "\""
        << ",\"sample_count\":" << s.sample_count
        << ",\"sign_mismatch_count\":" << s.sign_mismatch_count
        << ",\"false_inside_count\":" << s.false_inside_count
        << ",\"false_outside_count\":" << s.false_outside_count
        << ",\"p95_abs_error\":" << s.p95_abs_error
        << ",\"p95_normal_angle_error_deg\":"
        << s.p95_normal_angle_error_deg
        << ",\"average_block_level\":" << s.average_block_level
        << ",\"top_block_ids\":[";
    for (std::size_t j = 0; j < s.top_block_ids.size(); ++j) {
      if (j > 0) {
        out << ",";
      }
      out << s.top_block_ids[j];
    }
    out << "]}";
  }
  out << "]";
  return out.str();
}

std::string subgridCsvValues(const MismatchCellSampleDiagnostic& sample) {
  std::ostringstream out;
  bool first = true;
  for (const auto& item : sample.local_subgrid_results) {
    if (!first) {
      out << ";";
    }
    first = false;
    out << item.first << ":" << item.second.phi << ":"
        << item.second.sign << ":"
        << (item.second.fixed_mismatch ? 1 : 0) << ":"
        << (item.second.remaining_mismatch ? 1 : 0);
  }
  return out.str();
}

}  // namespace

std::string MismatchCellReportWriter::toJson(
    const MismatchCellForensicsResult& result) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"" << result.schema_id << "\",\n";
  out << "  \"case_id\": \"" << jsonEscape(result.case_id) << "\",\n";
  out << "  \"sample_count\": " << result.sample_count << ",\n";
  out << "  \"sign_mismatch_count\": " << result.sign_mismatch_count << ",\n";
  out << "  \"false_inside_count\": " << result.false_inside_count << ",\n";
  out << "  \"false_outside_count\": " << result.false_outside_count << ",\n";
  out << "  \"p95_abs_error\": " << result.p95_abs_error << ",\n";
  out << "  \"corner_pattern_all_positive_count\": "
      << result.corner_pattern_all_positive_count << ",\n";
  out << "  \"corner_pattern_all_negative_count\": "
      << result.corner_pattern_all_negative_count << ",\n";
  out << "  \"corner_pattern_mixed_sign_count\": "
      << result.corner_pattern_mixed_sign_count << ",\n";
  out << "  \"corner_pattern_has_near_zero_corner_count\": "
      << result.corner_pattern_has_near_zero_corner_count << ",\n";
  out << "  \"corner_pattern_checkerboard_like_count\": "
      << result.corner_pattern_checkerboard_like_count << ",\n";
  out << "  \"corner_pattern_single_opposite_corner_count\": "
      << result.corner_pattern_single_opposite_corner_count << ",\n";
  out << "  \"corner_pattern_edge_crossing_count\": "
      << result.corner_pattern_edge_crossing_count << ",\n";
  out << "  \"corner_pattern_face_crossing_count\": "
      << result.corner_pattern_face_crossing_count << ",\n";
  out << "  \"surface_crossing_with_same_sign_corners_count\": "
      << result.surface_crossing_with_same_sign_corners_count << ",\n";
  out << "  \"surface_crossing_with_mixed_corners_count\": "
      << result.surface_crossing_with_mixed_corners_count << ",\n";
  out << "  \"no_surface_crossing_but_sign_mismatch_count\": "
      << result.no_surface_crossing_but_sign_mismatch_count << ",\n";
  out << "  \"block_lookup_suspicious_count\": "
      << result.block_lookup_suspicious_count << ",\n";
  out << "  \"reference_sign_suspicious_count\": "
      << result.reference_sign_suspicious_count << ",\n";
  out << "  \"block_boundary_mismatch_count\": "
      << result.block_boundary_mismatch_count << ",\n";
  out << "  \"mixed_level_boundary_mismatch_count\": "
      << result.mixed_level_boundary_mismatch_count << ",\n";
  out << "  \"false_inside_major_corner_pattern\": \""
      << jsonEscape(result.false_inside_major_corner_pattern) << "\",\n";
  out << "  \"false_outside_major_corner_pattern\": \""
      << jsonEscape(result.false_outside_major_corner_pattern) << "\",\n";
  out << "  \"cell_resolution_insufficient_likely\": "
      << boolText(result.cell_resolution_insufficient_likely) << ",\n";
  out << "  \"subgrid_summaries\": "
      << subgridJson(result.subgrid_summaries) << ",\n";
  out << "  \"corner_pattern_stats\": "
      << statsJson(result.corner_pattern_stats) << ",\n";
  out << "  \"zero_crossing_stats\": "
      << statsJson(result.zero_crossing_stats) << "\n";
  out << "}\n";
  return out.str();
}

std::string MismatchCellReportWriter::toCSV(
    const MismatchCellForensicsResult& result) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "sample_id,x,y,z,reference_phi,sdf_phi,abs_error,reference_sign,"
         "sdf_sign,false_inside,false_outside,nearest_triangle_id,"
         "nearest_x,nearest_y,nearest_z,normal_x,normal_y,normal_z,"
         "block_id,block_level,block_min_x,block_min_y,block_min_z,"
         "block_max_x,block_max_y,block_max_z,block_nx,block_ny,block_nz,"
         "local_i,local_j,local_k,local_u,local_v,local_w,"
         "distance_to_cell_boundary,block_boundary_cell,"
         "mixed_level_boundary_cell,contact_band_block,"
         "coverage_promoted_block,far_field_block,provenance_exists,"
         "corner_phi_000,corner_phi_100,corner_phi_010,corner_phi_110,"
         "corner_phi_001,corner_phi_101,corner_phi_011,corner_phi_111,"
         "exact_corner_phi_000,exact_corner_phi_100,exact_corner_phi_010,"
         "exact_corner_phi_110,exact_corner_phi_001,exact_corner_phi_101,"
         "exact_corner_phi_011,exact_corner_phi_111,exact_center_phi,"
         "exact_stencil_phi,corner_sign_pattern,corner_pattern_category,"
         "positive_corner_count,negative_corner_count,near_zero_corner_count,"
         "trilinear_phi,exact_bvh_phi,triangle_cell_aabb_overlap,"
         "expanded_triangle_cell_aabb_overlap,box_triangle_approx_distance,"
         "nearest_surface_point_inside_cell,likely_zero_crossing_inside_cell,"
         "zero_crossing_category,reference_sign_suspicious,"
         "block_lookup_suspicious,local_subgrid_results\n";
  for (const MismatchCellSampleDiagnostic& s : result.samples) {
    out << s.sample_id << "," << s.point.x << "," << s.point.y << ","
        << s.point.z << "," << s.reference_phi << "," << s.sdf_phi
        << "," << s.abs_error << "," << s.reference_sign << ","
        << s.sdf_sign << "," << boolText(s.false_inside) << ","
        << boolText(s.false_outside) << "," << s.nearest_triangle_id
        << "," << s.nearest_point_on_triangle.x << ","
        << s.nearest_point_on_triangle.y << ","
        << s.nearest_point_on_triangle.z << ","
        << s.nearest_triangle_normal.x << ","
        << s.nearest_triangle_normal.y << ","
        << s.nearest_triangle_normal.z << "," << s.block_id << ","
        << s.block_level << "," << s.block_aabb.min.x << ","
        << s.block_aabb.min.y << "," << s.block_aabb.min.z << ","
        << s.block_aabb.max.x << "," << s.block_aabb.max.y << ","
        << s.block_aabb.max.z << "," << s.block_nx << ","
        << s.block_ny << "," << s.block_nz << "," << s.local_i << ","
        << s.local_j << "," << s.local_k << "," << s.local_u << ","
        << s.local_v << "," << s.local_w << ","
        << s.distance_to_cell_boundary << ","
        << boolText(s.block_boundary_cell) << ","
        << boolText(s.mixed_level_boundary_cell) << ","
        << boolText(s.contact_band_block) << ","
        << boolText(s.coverage_promoted_block) << ","
        << boolText(s.far_field_block) << ","
        << boolText(s.provenance_exists);
    for (double value : s.corner_phi) {
      out << "," << value;
    }
    for (double value : s.exact_corner_phi) {
      out << "," << value;
    }
    out << "," << s.exact_center_phi << ","
        << csvEscape(joinDoubles(s.exact_stencil_phi)) << ","
        << csvEscape(s.corner_sign_pattern) << ","
        << csvEscape(s.corner_pattern_category) << ","
        << s.positive_corner_count << "," << s.negative_corner_count
        << "," << s.near_zero_corner_count << "," << s.trilinear_phi
        << "," << s.exact_bvh_phi << ","
        << boolText(s.triangle_cell_aabb_overlap) << ","
        << boolText(s.expanded_triangle_cell_aabb_overlap) << ","
        << s.box_triangle_approx_distance << ","
        << boolText(s.nearest_surface_point_inside_cell) << ","
        << boolText(s.likely_zero_crossing_inside_cell) << ","
        << csvEscape(s.zero_crossing_category) << ","
        << boolText(s.reference_sign_suspicious) << ","
        << boolText(s.block_lookup_suspicious) << ","
        << csvEscape(subgridCsvValues(s)) << "\n";
  }
  return out.str();
}

std::string MismatchCellReportWriter::toMarkdown(
    const MismatchCellForensicsResult& result) {
  std::ostringstream out;
  out << std::setprecision(12);
  out << "# Mismatch Cell Forensics\n\n";
  out << "| Metric | Value |\n";
  out << "|---|---:|\n";
  out << "| sample_count | " << result.sample_count << " |\n";
  out << "| sign_mismatch_count | " << result.sign_mismatch_count << " |\n";
  out << "| false_inside_count | " << result.false_inside_count << " |\n";
  out << "| false_outside_count | " << result.false_outside_count << " |\n";
  out << "| p95_abs_error | " << result.p95_abs_error << " |\n";
  out << "| corner_pattern_all_positive | "
      << result.corner_pattern_all_positive_count << " |\n";
  out << "| corner_pattern_all_negative | "
      << result.corner_pattern_all_negative_count << " |\n";
  out << "| corner_pattern_mixed_sign | "
      << result.corner_pattern_mixed_sign_count << " |\n";
  out << "| surface_crossing_with_same_sign_corners | "
      << result.surface_crossing_with_same_sign_corners_count << " |\n";
  out << "| surface_crossing_with_mixed_corners | "
      << result.surface_crossing_with_mixed_corners_count << " |\n";
  out << "| block_boundary_mismatch | "
      << result.block_boundary_mismatch_count << " |\n";
  out << "| mixed_level_boundary_mismatch | "
      << result.mixed_level_boundary_mismatch_count << " |\n";
  out << "| cell_resolution_insufficient_likely | "
      << boolText(result.cell_resolution_insufficient_likely) << " |\n\n";

  out << "## Local Subgrid\n\n";
  out << "| subgrid | fixed_count | remaining_mismatch_count |\n";
  out << "|---:|---:|---:|\n";
  for (const MismatchCellSubgridSummary& s : result.subgrid_summaries) {
    out << "| " << s.subgrid << " | " << s.fixed_count << " | "
        << s.remaining_mismatch_count << " |\n";
  }

  out << "\n## Corner Pattern Stats\n\n";
  out << "| pattern | samples | false_inside | false_outside | p95_abs_error | average_level | top_blocks |\n";
  out << "|---|---:|---:|---:|---:|---:|---|\n";
  for (const MismatchCellPatternStats& s : result.corner_pattern_stats) {
    out << "| " << s.key << " | " << s.sample_count << " | "
        << s.false_inside_count << " | " << s.false_outside_count
        << " | " << s.p95_abs_error << " | "
        << s.average_block_level << " | " << joinInts(s.top_block_ids)
        << " |\n";
  }

  out << "\n## Zero-Crossing Stats\n\n";
  out << "| category | samples | false_inside | false_outside | p95_abs_error | average_level | top_blocks |\n";
  out << "|---|---:|---:|---:|---:|---:|---|\n";
  for (const MismatchCellPatternStats& s : result.zero_crossing_stats) {
    out << "| " << s.key << " | " << s.sample_count << " | "
        << s.false_inside_count << " | " << s.false_outside_count
        << " | " << s.p95_abs_error << " | "
        << s.average_block_level << " | " << joinInts(s.top_block_ids)
        << " |\n";
  }
  return out.str();
}

bool MismatchCellReportWriter::writeJson(
    const std::filesystem::path& path,
    const MismatchCellForensicsResult& result) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << toJson(result);
  return true;
}

bool MismatchCellReportWriter::writeCSV(
    const std::filesystem::path& path,
    const MismatchCellForensicsResult& result) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << toCSV(result);
  return true;
}

bool MismatchCellReportWriter::writeMarkdown(
    const std::filesystem::path& path,
    const MismatchCellForensicsResult& result) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << toMarkdown(result);
  return true;
}

}  // namespace adasdf
