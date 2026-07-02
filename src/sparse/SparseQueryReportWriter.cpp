#include "adasdf/sparse/SparseQueryReportWriter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace adasdf {
namespace {

std::string boolText(bool value) {
  return value ? "true" : "false";
}

std::string csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped.push_back(ch);
    }
  }
  escaped += "\"";
  return escaped;
}

bool prepareFile(
    const std::string& path,
    std::ofstream& file,
    std::string* error_message) {
  const std::filesystem::path out_path(path);
  if (!out_path.parent_path().empty()) {
    std::filesystem::create_directories(out_path.parent_path());
  }
  file.open(out_path);
  if (!file) {
    if (error_message) {
      *error_message = "failed to open output file: " + path;
    }
    return false;
  }
  file << std::setprecision(10);
  return true;
}

void writeVectorCsv(std::ostream& out, const Vector3& v) {
  out << v.x << "," << v.y << "," << v.z;
}

}  // namespace

std::string SparseQueryReportWriter::queryToMarkdown(
    const SparseSDFQueryResult& result) {
  std::ostringstream out;
  out << "# Sparse SDF Query Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Colliding: " << boolText(result.colliding) << "\n";
  out << "- Sample count: " << result.stats.sample_count << "\n";
  out << "- Queried count: " << result.stats.queried_count << "\n";
  out << "- Result count: " << result.stats.result_count << "\n";
  out << "- Min phi: " << result.stats.min_phi << "\n";
  out << "- Min effective phi: " << result.stats.min_effective_phi << "\n";
  out << "- Early exit: " << boolText(result.stats.early_exit_triggered) << "\n";
  out << "- Elapsed ms: " << result.stats.elapsed_ms << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "\nThis is a sparse point-to-SDF query report, not a full contact "
         "manifold or solver contact set.\n";
  return out.str();
}

std::string SparseQueryReportWriter::queryToJson(
    const SparseSDFQueryResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"colliding\": " << boolText(result.colliding) << ",\n";
  out << "  \"sample_count\": " << result.stats.sample_count << ",\n";
  out << "  \"queried_count\": " << result.stats.queried_count << ",\n";
  out << "  \"result_count\": " << result.stats.result_count << ",\n";
  out << "  \"min_phi\": " << result.stats.min_phi << ",\n";
  out << "  \"min_effective_phi\": " << result.stats.min_effective_phi << ",\n";
  out << "  \"early_exit\": " << boolText(result.stats.early_exit_triggered)
      << ",\n";
  out << "  \"elapsed_ms\": " << result.stats.elapsed_ms << "\n";
  out << "}\n";
  return out.str();
}

bool SparseQueryReportWriter::writeQueryCSV(
    const std::string& path,
    const SparseSDFQueryResult& result,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << "sample_id,x,y,z,radius,phi,effective_phi,colliding,"
          "normal_x,normal_y,normal_z,object_id,link_id,group_id,label\n";
  for (const SparseSDFSampleResult& sample : result.samples) {
    file << sample.sample_id << ","
         << sample.position.x << ","
         << sample.position.y << ","
         << sample.position.z << ","
         << sample.radius << ","
         << sample.phi << ","
         << sample.effective_phi << ","
         << boolText(sample.colliding) << ",";
    writeVectorCsv(file, sample.has_normal ? sample.normal : Vector3{});
    file << "," << sample.object_id
         << "," << sample.link_id
         << "," << sample.group_id
         << "," << csvField(sample.label) << "\n";
  }
  return true;
}

std::string SparseQueryReportWriter::collisionToMarkdown(
    const SparseCollisionResult& result) {
  std::ostringstream out;
  out << "# Sparse Collision Query Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Colliding: " << boolText(result.colliding) << "\n";
  out << "- Min phi: " << result.min_phi << "\n";
  out << "- Min effective phi: " << result.min_effective_phi << "\n";
  out << "- First hit sample id: " << result.first_hit_sample_id << "\n";
  out << "- Samples: " << result.sample_count << "\n";
  out << "- Queried samples: " << result.queried_count << "\n";
  out << "- Early exit: " << boolText(result.early_exit_triggered) << "\n";
  out << "- Violations: " << result.violations.size() << "\n";
  out << "- Elapsed ms: " << result.elapsed_ms << "\n\n";
  out << "Collision-only and clearance sparse queries are not full contact "
         "manifolds and do not generate solver constraints.\n";
  return out.str();
}

std::string SparseQueryReportWriter::collisionToJson(
    const SparseCollisionResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"colliding\": " << boolText(result.colliding) << ",\n";
  out << "  \"min_phi\": " << result.min_phi << ",\n";
  out << "  \"min_effective_phi\": " << result.min_effective_phi << ",\n";
  out << "  \"first_hit_sample_id\": " << result.first_hit_sample_id << ",\n";
  out << "  \"sample_count\": " << result.sample_count << ",\n";
  out << "  \"queried_count\": " << result.queried_count << ",\n";
  out << "  \"early_exit\": " << boolText(result.early_exit_triggered) << ",\n";
  out << "  \"violation_count\": " << result.violations.size() << ",\n";
  out << "  \"elapsed_ms\": " << result.elapsed_ms << "\n";
  out << "}\n";
  return out.str();
}

std::string SparseQueryReportWriter::candidatesToMarkdown(
    const ContactCandidateReductionResult& result) {
  std::ostringstream out;
  out << "# Contact Candidate Report\n\n";
  out << "- Input samples: " << result.input_count << "\n";
  out << "- Threshold candidates: " << result.threshold_candidate_count << "\n";
  out << "- Reduced candidates: " << result.reduced_count << "\n\n";
  out << "Candidates are a deterministic Top-K reduced set for downstream "
         "contact handling, not a full manifold or solver constraints.\n";
  return out.str();
}

std::string SparseQueryReportWriter::candidatesToJson(
    const ContactCandidateReductionResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"input_count\": " << result.input_count << ",\n";
  out << "  \"threshold_candidate_count\": "
      << result.threshold_candidate_count << ",\n";
  out << "  \"reduced_count\": " << result.reduced_count << "\n";
  out << "}\n";
  return out.str();
}

bool SparseQueryReportWriter::writeCandidatesCSV(
    const std::string& path,
    const ContactCandidateReductionResult& result,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << "rank,sample_id,x,y,z,radius,phi,effective_phi,penetration_depth,"
          "normal_x,normal_y,normal_z,object_id,link_id,group_id,label\n";
  for (const ContactCandidate& candidate : result.candidates) {
    file << candidate.rank << ","
         << candidate.sample_id << ","
         << candidate.point.x << ","
         << candidate.point.y << ","
         << candidate.point.z << ","
         << candidate.radius << ","
         << candidate.phi << ","
         << candidate.effective_phi << ","
         << candidate.penetration_depth << ",";
    writeVectorCsv(file, candidate.has_normal ? candidate.normal : Vector3{});
    file << "," << candidate.object_id
         << "," << candidate.link_id
         << "," << candidate.group_id
         << "," << csvField(candidate.label) << "\n";
  }
  return true;
}

}  // namespace adasdf
