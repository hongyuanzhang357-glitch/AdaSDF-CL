#include "adasdf/audit/SDFAccuracyReportWriter.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace adasdf {
namespace {

std::string jsonEscape(const std::string& text) {
  std::ostringstream out;
  for (char ch : text) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
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
        out << ch;
        break;
    }
  }
  return out.str();
}

std::string pathString(const std::filesystem::path& path) {
  return path.string();
}

const char* boolText(bool value) {
  return value ? "true" : "false";
}

std::string csvEscape(const std::string& text) {
  if (text.find_first_of(",\"\n\r") == std::string::npos) {
    return text;
  }
  std::string out = "\"";
  for (char ch : text) {
    if (ch == '"') {
      out += "\"\"";
    } else {
      out += ch;
    }
  }
  out += "\"";
  return out;
}

void writeMetricJson(std::ostream& out, const char* name, double value, bool comma) {
  out << "  \"" << name << "\": " << std::setprecision(17) << value;
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeMetricJson(
    std::ostream& out,
    const char* name,
    std::size_t value,
    bool comma) {
  out << "  \"" << name << "\": " << value;
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeMetricJson(std::ostream& out, const char* name, bool value, bool comma) {
  out << "  \"" << name << "\": " << boolText(value);
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeBinsJson(
    std::ostream& out,
    const char* name,
    const std::vector<SDFAccuracyBinStats>& bins,
    bool comma) {
  out << "  \"" << name << "\": [\n";
  for (std::size_t i = 0; i < bins.size(); ++i) {
    const SDFAccuracyBinStats& bin = bins[i];
    out << "    {"
        << "\"key\": \"" << jsonEscape(bin.key) << "\""
        << ", \"label\": \"" << jsonEscape(bin.label) << "\""
        << ", \"sample_count\": " << bin.sample_count
        << ", \"sign_mismatch_count\": " << bin.sign_mismatch_count
        << ", \"sign_mismatch_rate\": " << std::setprecision(17)
        << bin.sign_mismatch_rate
        << ", \"false_inside_count\": " << bin.false_inside_count
        << ", \"false_outside_count\": " << bin.false_outside_count
        << ", \"p95_abs_error\": " << bin.p95_abs_error
        << ", \"p95_normal_angle_error_deg\": "
        << bin.p95_normal_angle_error_deg << "}";
    if (i + 1 < bins.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]";
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeBinsMarkdown(
    std::ostream& out,
    const char* title,
    const std::vector<SDFAccuracyBinStats>& bins) {
  if (bins.empty()) {
    return;
  }
  out << "## " << title << "\n\n";
  out << "| key | samples | sign mismatch | sign rate | false inside | false outside | p95 abs | normal p95 deg |\n";
  out << "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n";
  for (const SDFAccuracyBinStats& bin : bins) {
    out << "| " << bin.label << " | " << bin.sample_count << " | "
        << bin.sign_mismatch_count << " | " << bin.sign_mismatch_rate
        << " | " << bin.false_inside_count << " | "
        << bin.false_outside_count << " | " << bin.p95_abs_error
        << " | " << bin.p95_normal_angle_error_deg << " |\n";
  }
  out << "\n";
}

}  // namespace

std::string SDFAccuracyReportWriter::toJson(
    const SDFAccuracyAuditResult& result) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"" << jsonEscape(result.schema_id) << "\",\n";
  out << "  \"case_id\": \"" << jsonEscape(result.case_id) << "\",\n";
  out << "  \"input_stl\": \"" << jsonEscape(pathString(result.input_stl))
      << "\",\n";
  out << "  \"input_sdf\": \"" << jsonEscape(pathString(result.input_sdf))
      << "\",\n";
  writeMetricJson(out, "sample_count_total", result.sample_count_total, true);
  writeMetricJson(
      out, "near_surface_sample_count", result.near_surface_sample_count, true);
  writeMetricJson(out, "out_of_domain_count", result.out_of_domain_count, true);
  writeMetricJson(out, "query_failed_count", result.query_failed_count, true);
  writeMetricJson(out, "max_abs_error", result.max_abs_error, true);
  writeMetricJson(out, "mean_abs_error", result.mean_abs_error, true);
  writeMetricJson(out, "rms_abs_error", result.rms_abs_error, true);
  writeMetricJson(out, "p50_abs_error", result.p50_abs_error, true);
  writeMetricJson(out, "p95_abs_error", result.p95_abs_error, true);
  writeMetricJson(out, "p99_abs_error", result.p99_abs_error, true);
  writeMetricJson(out, "sign_mismatch_count", result.sign_mismatch_count, true);
  writeMetricJson(out, "sign_mismatch_rate", result.sign_mismatch_rate, true);
  writeMetricJson(
      out,
      "near_surface_sign_mismatch_count",
      result.near_surface_sign_mismatch_count,
      true);
  writeMetricJson(out, "false_inside_count", result.false_inside_count, true);
  writeMetricJson(out, "false_outside_count", result.false_outside_count, true);
  writeMetricJson(
      out, "normal_audit_enabled", result.normal_audit_enabled, true);
  writeMetricJson(out, "normal_check_count", result.normal_check_count, true);
  writeMetricJson(
      out,
      "mean_normal_angle_error_deg",
      result.mean_normal_angle_error_deg,
      true);
  writeMetricJson(
      out,
      "p95_normal_angle_error_deg",
      result.p95_normal_angle_error_deg,
      true);
  writeMetricJson(
      out,
      "max_normal_angle_error_deg",
      result.max_normal_angle_error_deg,
      true);
  writeMetricJson(out, "normal_flip_count", result.normal_flip_count, true);
  writeMetricJson(
      out,
      "near_surface_normal_flip_count",
      result.near_surface_normal_flip_count,
      true);
  writeMetricJson(out, "sdf_query_time_ms", result.sdf_query_time_ms, true);
  writeMetricJson(out, "sdf_normal_time_ms", result.sdf_normal_time_ms, true);
  writeMetricJson(
      out, "exact_reference_time_ms", result.exact_reference_time_ms, true);
  writeMetricJson(out, "sdf_query_count", result.sdf_query_count, true);
  writeMetricJson(
      out,
      "exact_reference_query_count",
      result.exact_reference_query_count,
      true);
  writeMetricJson(out, "ns_per_sdf_query", result.ns_per_sdf_query, true);
  writeMetricJson(
      out, "ns_per_reference_query", result.ns_per_reference_query, true);
  writeMetricJson(
      out,
      "near_surface_p95_abs_error_limit",
      result.near_surface_p95_abs_error_limit,
      true);
  writeMetricJson(
      out,
      "near_surface_max_abs_error_limit",
      result.near_surface_max_abs_error_limit,
      true);
  writeMetricJson(
      out,
      "p95_normal_angle_error_deg_limit",
      result.p95_normal_angle_error_deg_limit,
      true);
  writeMetricJson(
      out,
      "near_surface_quality_passed",
      result.near_surface_quality_passed,
      true);
  writeMetricJson(out, "sign_quality_passed", result.sign_quality_passed, true);
  writeMetricJson(
      out, "normal_quality_passed", result.normal_quality_passed, true);
  writeMetricJson(out, "full_quality_passed", result.full_quality_passed, true);
  writeBinsJson(out, "offset_bins", result.offset_bins, true);
  writeBinsJson(out, "reference_phi_bins", result.reference_phi_bins, true);
  writeBinsJson(out, "block_level_bins", result.block_level_bins, true);
  writeBinsJson(out, "block_source_bins", result.block_source_bins, true);
  writeBinsJson(out, "query_source_bins", result.query_source_bins, true);
  out << "  \"samples\": [\n";
  for (std::size_t i = 0; i < result.samples.size(); ++i) {
    const SDFAccuracyAuditSample& s = result.samples[i];
    out << "    {"
        << "\"sample_id\": " << s.sample_id
        << ", \"surface_sample_id\": " << s.surface_sample_id
        << ", \"triangle_index\": " << s.triangle_index
        << ", \"offset\": " << s.offset << ", \"x\": " << s.point.x
        << ", \"y\": " << s.point.y << ", \"z\": " << s.point.z
        << ", \"reference_phi\": " << s.reference_phi
        << ", \"sdf_phi\": " << s.sdf_phi
        << ", \"abs_error\": " << s.abs_error
        << ", \"near_surface\": " << boolText(s.near_surface)
        << ", \"out_of_domain\": " << boolText(s.out_of_domain)
        << ", \"query_failed\": " << boolText(s.query_failed)
        << ", \"sign_mismatch\": " << boolText(s.sign_mismatch)
        << ", \"false_inside\": " << boolText(s.false_inside)
        << ", \"false_outside\": " << boolText(s.false_outside)
        << ", \"reference_sign\": " << s.reference_sign
        << ", \"sdf_sign\": " << s.sdf_sign
        << ", \"nearest_triangle_id\": " << s.nearest_triangle_id
        << ", \"block_id\": " << s.block_id
        << ", \"block_level\": " << s.block_level
        << ", \"local_i\": " << s.local_i
        << ", \"local_j\": " << s.local_j
        << ", \"local_k\": " << s.local_k
        << ", \"local_u\": " << s.local_u
        << ", \"local_v\": " << s.local_v
        << ", \"local_w\": " << s.local_w
        << ", \"corner_sign_pattern\": \""
        << jsonEscape(s.corner_sign_pattern) << "\""
        << ", \"block_source\": \"" << jsonEscape(s.block_source) << "\""
        << ", \"query_source\": \"" << jsonEscape(s.query_source) << "\""
        << ", \"normal_checked\": " << boolText(s.normal_checked)
        << ", \"normal_angle_error_deg\": " << s.normal_angle_error_deg
        << ", \"normal_flip\": " << boolText(s.normal_flip) << "}";
    if (i + 1 < result.samples.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string SDFAccuracyReportWriter::toMarkdown(
    const SDFAccuracyAuditResult& result) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "# SDF Near-Surface Accuracy Audit\n\n";
  if (!result.case_id.empty()) {
    out << "- Case id: " << result.case_id << "\n";
  }
  out << "- Input STL: " << pathString(result.input_stl) << "\n";
  out << "- Input SDF: " << pathString(result.input_sdf) << "\n";
  out << "- Total samples: " << result.sample_count_total << "\n";
  out << "- Near-surface samples: " << result.near_surface_sample_count << "\n";
  out << "- Query failures: " << result.query_failed_count << "\n";
  out << "- Out-of-domain samples: " << result.out_of_domain_count << "\n\n";

  out << "## Distance Error\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| max_abs_error | " << result.max_abs_error << " |\n";
  out << "| mean_abs_error | " << result.mean_abs_error << " |\n";
  out << "| rms_abs_error | " << result.rms_abs_error << " |\n";
  out << "| p50_abs_error | " << result.p50_abs_error << " |\n";
  out << "| p95_abs_error | " << result.p95_abs_error << " |\n";
  out << "| p99_abs_error | " << result.p99_abs_error << " |\n\n";

  out << "## Sign\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| sign_mismatch_count | " << result.sign_mismatch_count << " |\n";
  out << "| sign_mismatch_rate | " << result.sign_mismatch_rate << " |\n";
  out << "| near_surface_sign_mismatch_count | "
      << result.near_surface_sign_mismatch_count << " |\n";
  out << "| false_inside_count | " << result.false_inside_count << " |\n";
  out << "| false_outside_count | " << result.false_outside_count << " |\n\n";

  out << "## Normal\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| normal_audit_enabled | " << boolText(result.normal_audit_enabled)
      << " |\n";
  out << "| normal_check_count | " << result.normal_check_count << " |\n";
  out << "| mean_normal_angle_error_deg | "
      << result.mean_normal_angle_error_deg << " |\n";
  out << "| p95_normal_angle_error_deg | "
      << result.p95_normal_angle_error_deg << " |\n";
  out << "| max_normal_angle_error_deg | "
      << result.max_normal_angle_error_deg << " |\n";
  out << "| normal_flip_count | " << result.normal_flip_count << " |\n";
  out << "| near_surface_normal_flip_count | "
      << result.near_surface_normal_flip_count << " |\n\n";

  out << "## Timing\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| sdf_query_time_ms | " << result.sdf_query_time_ms << " |\n";
  out << "| sdf_normal_time_ms | " << result.sdf_normal_time_ms << " |\n";
  out << "| exact_reference_time_ms | " << result.exact_reference_time_ms
      << " |\n";
  out << "| ns_per_sdf_query | " << result.ns_per_sdf_query << " |\n";
  out << "| ns_per_reference_query | " << result.ns_per_reference_query << " |\n\n";

  out << "## Quality Gates\n\n";
  out << "| gate | passed |\n";
  out << "| --- | --- |\n";
  out << "| near_surface_quality_passed | "
      << boolText(result.near_surface_quality_passed) << " |\n";
  out << "| sign_quality_passed | " << boolText(result.sign_quality_passed)
      << " |\n";
  out << "| normal_quality_passed | "
      << boolText(result.normal_quality_passed) << " |\n";
  out << "| full_quality_passed | " << boolText(result.full_quality_passed)
      << " |\n";
  out << "\n";

  writeBinsMarkdown(out, "Offset Bins", result.offset_bins);
  writeBinsMarkdown(out, "Reference Phi Bins", result.reference_phi_bins);
  writeBinsMarkdown(out, "Block Level Bins", result.block_level_bins);
  writeBinsMarkdown(out, "Block Source Bins", result.block_source_bins);
  writeBinsMarkdown(out, "Query Source Bins", result.query_source_bins);
  return out.str();
}

bool SDFAccuracyReportWriter::writeJson(
    const std::filesystem::path& path,
    const SDFAccuracyAuditResult& result) {
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toJson(result);
  return true;
}

bool SDFAccuracyReportWriter::writeCSV(
    const std::filesystem::path& path,
    const SDFAccuracyAuditResult& result) {
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << "sample_id,surface_sample_id,triangle_index,offset,x,y,z,"
          "reference_phi,sdf_phi,abs_error,near_surface,out_of_domain,"
          "query_failed,sign_mismatch,false_inside,false_outside,"
          "reference_sign,sdf_sign,nearest_triangle_id,block_id,block_level,"
          "local_i,local_j,local_k,local_u,local_v,local_w,"
          "corner_sign_pattern,block_source,query_source,"
          "normal_checked,normal_angle_error_deg,normal_flip,case_id\n";
  file << std::setprecision(17);
  for (const SDFAccuracyAuditSample& s : result.samples) {
    file << s.sample_id << "," << s.surface_sample_id << ","
         << s.triangle_index << "," << s.offset << "," << s.point.x << ","
         << s.point.y << "," << s.point.z << "," << s.reference_phi << ","
         << s.sdf_phi << "," << s.abs_error << ","
         << boolText(s.near_surface) << "," << boolText(s.out_of_domain)
         << "," << boolText(s.query_failed) << ","
         << boolText(s.sign_mismatch) << "," << boolText(s.false_inside)
         << "," << boolText(s.false_outside) << "," << s.reference_sign
         << "," << s.sdf_sign << "," << s.nearest_triangle_id << ","
         << s.block_id << "," << s.block_level << "," << s.local_i << ","
         << s.local_j << "," << s.local_k << "," << s.local_u << ","
         << s.local_v << "," << s.local_w << ","
         << csvEscape(s.corner_sign_pattern) << ","
         << csvEscape(s.block_source) << "," << csvEscape(s.query_source)
         << ","
         << boolText(s.normal_checked) << "," << s.normal_angle_error_deg
         << "," << boolText(s.normal_flip) << ","
         << csvEscape(result.case_id) << "\n";
  }
  return true;
}

bool SDFAccuracyReportWriter::writeMismatchCSV(
    const std::filesystem::path& path,
    const SDFAccuracyAuditResult& result,
    std::size_t max_samples) {
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << "x,y,z,offset,reference_phi,sdf_phi,abs_error,reference_sign,"
          "sdf_sign,false_inside_or_false_outside,nearest_triangle_id,"
          "block_id,block_level,block_min_x,block_min_y,block_min_z,"
          "block_max_x,block_max_y,block_max_z,local_i,local_j,local_k,"
          "local_u,local_v,local_w,corner_phi_000,corner_phi_100,"
          "corner_phi_010,corner_phi_110,corner_phi_001,corner_phi_101,"
          "corner_phi_011,corner_phi_111,corner_sign_pattern,query_status,"
          "case_id\n";
  file << std::setprecision(17);
  std::size_t written = 0;
  for (const SDFAccuracyAuditSample& s : result.samples) {
    if (!s.sign_mismatch) {
      continue;
    }
    if (written >= max_samples) {
      break;
    }
    const std::string kind =
        s.false_inside ? "false_inside"
                       : (s.false_outside ? "false_outside" : "mismatch");
    const std::string status = s.query_failed ? "query_failed" : "ok";
    file << s.point.x << "," << s.point.y << "," << s.point.z << ","
         << s.offset << "," << s.reference_phi << "," << s.sdf_phi << ","
         << s.abs_error << "," << s.reference_sign << "," << s.sdf_sign
         << "," << kind << "," << s.nearest_triangle_id << ","
         << s.block_id << "," << s.block_level << "," << s.block_aabb.min.x
         << "," << s.block_aabb.min.y << "," << s.block_aabb.min.z << ","
         << s.block_aabb.max.x << "," << s.block_aabb.max.y << ","
         << s.block_aabb.max.z << "," << s.local_i << "," << s.local_j
         << "," << s.local_k << "," << s.local_u << "," << s.local_v
         << "," << s.local_w;
    for (double value : s.corner_phi) {
      file << "," << value;
    }
    file << "," << csvEscape(s.corner_sign_pattern) << ","
         << csvEscape(status) << "," << csvEscape(result.case_id) << "\n";
    ++written;
  }
  return true;
}

bool SDFAccuracyReportWriter::writeMarkdown(
    const std::filesystem::path& path,
    const SDFAccuracyAuditResult& result) {
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toMarkdown(result);
  return true;
}

}  // namespace adasdf
