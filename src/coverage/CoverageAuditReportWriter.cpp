#include "adasdf/coverage/CoverageAuditReportWriter.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "adasdf/contract/JsonContractWriter.h"

namespace adasdf {
namespace {

const char* boolText(bool value) {
  return value ? "true" : "false";
}

void createParent(const std::filesystem::path& path) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string missedByBlockJson(const CoverageAuditResult& result) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (int block_id : result.top_missed_block_ids) {
    const auto iter = result.missed_by_block_id.find(block_id);
    if (iter == result.missed_by_block_id.end()) {
      continue;
    }
    if (!first) {
      out << ",";
    }
    first = false;
    out << "{\"block_id\":" << block_id << ",\"missed_samples\":"
        << iter->second << "}";
  }
  out << "]";
  return out.str();
}

std::string missedByLevelJson(const CoverageAuditResult& result) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto& item : result.missed_by_level) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "{\"level\":" << item.first << ",\"missed_samples\":"
        << item.second << "}";
  }
  out << "]";
  return out.str();
}

std::string refinementJson(const CoverageDrivenRefinementResult& result) {
  std::ostringstream out;
  out << "{\"applied\":" << boolText(result.applied)
      << ",\"promotion_mode\":"
      << JsonContractWriter::quote(result.promotion_mode)
      << ",\"promoted_block_count\":" << result.promoted_block_count
      << ",\"promoted_cell_count\":" << result.promoted_cell_count
      << ",\"promoted_node_count\":" << result.promoted_node_count
      << ",\"resample_time_ms\":"
      << JsonContractWriter::number(result.resample_time_ms)
      << ",\"promoted_block_ids\":[";
  for (std::size_t i = 0; i < result.promoted_block_ids.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << result.promoted_block_ids[i];
  }
  out << "]}";
  return out.str();
}

}  // namespace

std::string CoverageAuditReportWriter::toJson(
    const CoverageAuditResult& result) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"adasdf.contact_band_coverage_audit.v1\",\n";
  out << "  \"total_samples\": " << result.total_samples << ",\n";
  out << "  \"near_surface_samples\": " << result.near_surface_samples
      << ",\n";
  out << "  \"covered_samples\": " << result.covered_samples << ",\n";
  out << "  \"missed_samples\": " << result.missed_samples << ",\n";
  out << "  \"missed_rate\": " << result.missed_rate << ",\n";
  out << "  \"coverage_passed\": " << boolText(result.coverage_passed)
      << ",\n";
  out << "  \"missed_by_level\": " << missedByLevelJson(result) << ",\n";
  out << "  \"missed_by_block\": " << missedByBlockJson(result) << "\n";
  out << "}\n";
  return out.str();
}

std::string CoverageAuditReportWriter::toMarkdown(
    const CoverageAuditResult& result) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "# Contact-Band Coverage Audit\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| total_samples | " << result.total_samples << " |\n";
  out << "| near_surface_samples | " << result.near_surface_samples << " |\n";
  out << "| covered_samples | " << result.covered_samples << " |\n";
  out << "| missed_samples | " << result.missed_samples << " |\n";
  out << "| missed_rate | " << result.missed_rate << " |\n";
  out << "| coverage_passed | " << boolText(result.coverage_passed) << " |\n\n";
  out << "## Top Missed Blocks\n\n";
  out << "| block_id | missed_samples |\n";
  out << "| ---: | ---: |\n";
  for (int block_id : result.top_missed_block_ids) {
    const auto iter = result.missed_by_block_id.find(block_id);
    if (iter != result.missed_by_block_id.end()) {
      out << "| " << block_id << " | " << iter->second << " |\n";
    }
  }
  out << "\n";
  return out.str();
}

std::string CoverageAuditReportWriter::toJson(
    const CoverageAuditRunReport& report) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"adasdf.coverage_refinement_run.v1\",\n";
  out << "  \"case_id\": " << JsonContractWriter::quote(report.case_id)
      << ",\n";
  out << "  \"near_band\": " << report.options.near_band << ",\n";
  out << "  \"surface_samples\": " << report.options.surface_samples
      << ",\n";
  out << "  \"iterations\": [\n";
  for (std::size_t i = 0; i < report.iterations.size(); ++i) {
    const CoverageAuditIteration& iter = report.iterations[i];
    out << "    {\"iteration\":" << iter.iteration
        << ",\"audit\":" << toJson(iter.audit)
        << ",\"refinement\":" << refinementJson(iter.refinement) << "}";
    if (i + 1 < report.iterations.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string CoverageAuditReportWriter::toMarkdown(
    const CoverageAuditRunReport& report) {
  std::ostringstream out;
  out << "# Coverage Refinement Run\n\n";
  if (!report.case_id.empty()) {
    out << "- Case id: " << report.case_id << "\n";
  }
  out << "- Near band: " << report.options.near_band << "\n";
  out << "- Surface samples: " << report.options.surface_samples << "\n\n";
  out << "| iteration | near samples | missed | missed rate | promoted blocks | promoted cells |\n";
  out << "| ---: | ---: | ---: | ---: | ---: | ---: |\n";
  for (const CoverageAuditIteration& iter : report.iterations) {
    out << "| " << iter.iteration << " | "
        << iter.audit.near_surface_samples << " | "
        << iter.audit.missed_samples << " | " << iter.audit.missed_rate
        << " | " << iter.refinement.promoted_block_count << " | "
        << iter.refinement.promoted_cell_count << " |\n";
  }
  out << "\n";
  if (!report.iterations.empty()) {
    out << CoverageAuditReportWriter::toMarkdown(report.iterations.back().audit);
  }
  return out.str();
}

bool CoverageAuditReportWriter::writeJson(
    const std::filesystem::path& path,
    const CoverageAuditResult& result) {
  createParent(path);
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toJson(result);
  return static_cast<bool>(file);
}

bool CoverageAuditReportWriter::writeMarkdown(
    const std::filesystem::path& path,
    const CoverageAuditResult& result) {
  createParent(path);
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toMarkdown(result);
  return static_cast<bool>(file);
}

bool CoverageAuditReportWriter::writeJson(
    const std::filesystem::path& path,
    const CoverageAuditRunReport& report) {
  createParent(path);
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toJson(report);
  return static_cast<bool>(file);
}

bool CoverageAuditReportWriter::writeMarkdown(
    const std::filesystem::path& path,
    const CoverageAuditRunReport& report) {
  createParent(path);
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << toMarkdown(report);
  return static_cast<bool>(file);
}

}  // namespace adasdf
