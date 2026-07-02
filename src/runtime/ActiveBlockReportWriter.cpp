#include "adasdf/runtime/ActiveBlockReportWriter.h"

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

std::string blockIdsJson(const std::vector<int>& block_ids) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < block_ids.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << block_ids[i];
  }
  out << "]";
  return out.str();
}

}  // namespace

std::string ActiveBlockReportWriter::selectionToMarkdown(
    const ActiveBlockSelectionResult& result) {
  std::ostringstream out;
  out << "# Active Block Selection Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Sample count: " << result.sample_count << "\n";
  out << "- Candidate samples: " << result.candidate_sample_count << "\n";
  out << "- Active blocks: " << result.block_ids.size() << "\n";
  out << "- Threshold: " << result.threshold << "\n";
  out << "- Selection band: " << result.selection_band << "\n";
  out << "- Extra margin: " << result.extra_margin << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "\nActive block selection is a contact-aware local expansion seed; "
         "it is not global SDF expansion.\n";
  return out.str();
}

std::string ActiveBlockReportWriter::selectionToJson(
    const ActiveBlockSelectionResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"sample_count\": " << result.sample_count << ",\n";
  out << "  \"candidate_sample_count\": "
      << result.candidate_sample_count << ",\n";
  out << "  \"active_block_count\": " << result.block_ids.size() << ",\n";
  out << "  \"threshold\": " << result.threshold << ",\n";
  out << "  \"selection_band\": " << result.selection_band << ",\n";
  out << "  \"extra_margin\": " << result.extra_margin << ",\n";
  out << "  \"block_ids\": " << blockIdsJson(result.block_ids) << "\n";
  out << "}\n";
  return out.str();
}

bool ActiveBlockReportWriter::writeSelectionCSV(
    const std::string& path,
    const ActiveBlockSelectionResult& result,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << "block_id\n";
  for (const int block_id : result.block_ids) {
    file << block_id << "\n";
  }
  return true;
}

std::string ActiveBlockReportWriter::queryToMarkdown(
    const ActiveBlockQueryResult& result) {
  std::ostringstream out;
  out << "# Active Block Query Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Colliding: " << boolText(result.colliding) << "\n";
  out << "- Sample count: " << result.stats.sample_count << "\n";
  out << "- Queried count: " << result.stats.queried_count << "\n";
  out << "- Result count: " << result.stats.result_count << "\n";
  out << "- Cache queries: " << result.stats.cache_query_count << "\n";
  out << "- Fallback queries: " << result.stats.fallback_query_count << "\n";
  out << "- Colliding count: " << result.stats.colliding_count << "\n";
  out << "- Min phi: " << result.stats.min_phi << "\n";
  out << "- Min effective phi: " << result.stats.min_effective_phi << "\n";
  out << "- Expanded blocks this call: "
      << result.stats.expansion_stats.expanded_block_count << "\n";
  out << "- Resident cache blocks: "
      << result.stats.cache_stats.block_count << "\n";
  out << "- Resident cache memory bytes: "
      << result.stats.cache_stats.memory_bytes << "\n";
  out << "- Cache hit rate: " << result.stats.cache_stats.hitRate() << "\n";
  out << "- Early exit: " << boolText(result.stats.early_exit_triggered) << "\n";
  out << "- Query time ms: " << result.stats.query_time_ms << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "\nThe active block cache expands only selected local blocks. "
         "Fallback rows use the model's direct query backend.\n";
  return out.str();
}

std::string ActiveBlockReportWriter::queryToJson(
    const ActiveBlockQueryResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"colliding\": " << boolText(result.colliding) << ",\n";
  out << "  \"sample_count\": " << result.stats.sample_count << ",\n";
  out << "  \"queried_count\": " << result.stats.queried_count << ",\n";
  out << "  \"result_count\": " << result.stats.result_count << ",\n";
  out << "  \"cache_query_count\": "
      << result.stats.cache_query_count << ",\n";
  out << "  \"fallback_query_count\": "
      << result.stats.fallback_query_count << ",\n";
  out << "  \"colliding_count\": " << result.stats.colliding_count << ",\n";
  out << "  \"cache_block_count\": "
      << result.stats.cache_stats.block_count << ",\n";
  out << "  \"cache_memory_bytes\": "
      << result.stats.cache_stats.memory_bytes << ",\n";
  out << "  \"cache_hit_rate\": "
      << result.stats.cache_stats.hitRate() << ",\n";
  out << "  \"query_time_ms\": " << result.stats.query_time_ms << "\n";
  out << "}\n";
  return out.str();
}

bool ActiveBlockReportWriter::writeQueryCSV(
    const std::string& path,
    const ActiveBlockQueryResult& result,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << "sample_id,x,y,z,radius,phi,effective_phi,colliding,source,"
          "normal_x,normal_y,normal_z,object_id,link_id,group_id,label\n";
  for (std::size_t i = 0; i < result.samples.size(); ++i) {
    const SparseSDFSampleResult& sample = result.samples[i];
    const std::string source =
        i < result.sample_sources.size() ? result.sample_sources[i] : "";
    file << sample.sample_id << ","
         << sample.position.x << ","
         << sample.position.y << ","
         << sample.position.z << ","
         << sample.radius << ","
         << sample.phi << ","
         << sample.effective_phi << ","
         << boolText(sample.colliding) << ","
         << csvField(source) << ",";
    writeVectorCsv(file, sample.has_normal ? sample.normal : Vector3{});
    file << "," << sample.object_id
         << "," << sample.link_id
         << "," << sample.group_id
         << "," << csvField(sample.label) << "\n";
  }
  return true;
}

std::string ActiveBlockReportWriter::expansionToMarkdown(
    const BlockExpansionResult& result) {
  std::ostringstream out;
  out << "# Active Block Expansion Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Requested blocks: " << result.stats.requested_block_count << "\n";
  out << "- Expanded blocks: " << result.stats.expanded_block_count << "\n";
  out << "- Cache hits: " << result.stats.cache_hit_count << "\n";
  out << "- Cache misses: " << result.stats.cache_miss_count << "\n";
  out << "- Skipped blocks: " << result.stats.skipped_block_count << "\n";
  out << "- Expanded memory bytes: "
      << result.stats.expanded_memory_bytes << "\n";
  out << "- Resident cache blocks: " << result.cache_stats.block_count << "\n";
  out << "- Resident cache memory bytes: "
      << result.cache_stats.memory_bytes << "\n";
  out << "- Expansion time ms: " << result.stats.expansion_time_ms << "\n";
  return out.str();
}

std::string ActiveBlockReportWriter::expansionToJson(
    const BlockExpansionResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"requested_block_count\": "
      << result.stats.requested_block_count << ",\n";
  out << "  \"expanded_block_count\": "
      << result.stats.expanded_block_count << ",\n";
  out << "  \"cache_hit_count\": " << result.stats.cache_hit_count << ",\n";
  out << "  \"cache_miss_count\": " << result.stats.cache_miss_count << ",\n";
  out << "  \"skipped_block_count\": "
      << result.stats.skipped_block_count << ",\n";
  out << "  \"expanded_memory_bytes\": "
      << result.stats.expanded_memory_bytes << ",\n";
  out << "  \"cache_block_count\": "
      << result.cache_stats.block_count << ",\n";
  out << "  \"cache_memory_bytes\": "
      << result.cache_stats.memory_bytes << ",\n";
  out << "  \"expansion_time_ms\": "
      << result.stats.expansion_time_ms << "\n";
  out << "}\n";
  return out.str();
}

}  // namespace adasdf
