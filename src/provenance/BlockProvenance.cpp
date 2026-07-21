#include "adasdf/provenance/BlockProvenance.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "adasdf/contract/JsonContractWriter.h"

namespace adasdf {
namespace {

std::string boolJson(bool value) {
  return JsonContractWriter::boolean(value);
}

std::string quote(const std::string& text) {
  return JsonContractWriter::quote(text);
}

std::size_t logicalNodeCount(int nx, int ny, int nz) {
  return static_cast<std::size_t>(std::max(0, nx)) *
         static_cast<std::size_t>(std::max(0, ny)) *
         static_cast<std::size_t>(std::max(0, nz));
}

std::unordered_map<int, AdaptiveTreeBlockSamplingStats> statsByBlockId(
    const AdaptiveBlockSDFBuildReport& report) {
  std::unordered_map<int, AdaptiveTreeBlockSamplingStats> out;
  for (const AdaptiveTreeBlockSamplingStats& stats :
       report.adaptive_tree_block_sampling_stats) {
    out[stats.block_id] = stats;
  }
  return out;
}

BlockProvenance makeBaseProvenance(
    int block_id,
    int level,
    bool near_surface,
    std::size_t logical_node_count,
    const std::unordered_map<int, AdaptiveTreeBlockSamplingStats>& stats,
    const std::string& sampling_mode,
    const std::string& marker_mode,
    double contact_band_width,
    double coverage_near_band) {
  BlockProvenance item;
  item.block_id = block_id;
  item.level = level;
  item.is_contact_band_block = near_surface;
  item.is_far_field_block = !near_surface;
  item.logical_node_count = logical_node_count;
  item.exact_node_count = near_surface ? logical_node_count : 0;
  item.predicted_node_count = near_surface ? 0 : logical_node_count;
  const auto iter = stats.find(block_id);
  if (iter != stats.end()) {
    item.is_contact_band_block = iter->second.contact_band;
    item.is_far_field_block = iter->second.far_field;
    item.exact_node_count = iter->second.exact_node_count;
    item.predicted_node_count = iter->second.predicted_node_count;
    item.logical_node_count = iter->second.logical_node_count > 0
                                  ? iter->second.logical_node_count
                                  : logical_node_count;
    item.has_exact_contact_cells = iter->second.exact_node_count > 0;
  } else {
    item.has_exact_contact_cells = item.exact_node_count > 0;
  }
  item.sampling_mode = sampling_mode;
  item.marker_mode = marker_mode;
  item.contact_band_width = contact_band_width;
  item.coverage_near_band = coverage_near_band;
  return item;
}

bool readBoolField(const std::string& object, const char* key, bool* value) {
  const std::regex pattern(
      std::string("\"") + key + "\"\\s*:\\s*(true|false)");
  std::smatch match;
  if (!std::regex_search(object, match, pattern)) {
    return false;
  }
  *value = match[1].str() == "true";
  return true;
}

bool readIntField(const std::string& object, const char* key, int* value) {
  const std::regex pattern(std::string("\"") + key + "\"\\s*:\\s*(-?\\d+)");
  std::smatch match;
  if (!std::regex_search(object, match, pattern)) {
    return false;
  }
  *value = std::stoi(match[1].str());
  return true;
}

bool readSizeField(
    const std::string& object,
    const char* key,
    std::size_t* value) {
  const std::regex pattern(std::string("\"") + key + "\"\\s*:\\s*(\\d+)");
  std::smatch match;
  if (!std::regex_search(object, match, pattern)) {
    return false;
  }
  *value = static_cast<std::size_t>(std::stoull(match[1].str()));
  return true;
}

bool readDoubleField(const std::string& object, const char* key, double* value) {
  const std::regex pattern(
      std::string("\"") + key +
      "\"\\s*:\\s*(-?(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:[eE][+-]?\\d+)?)");
  std::smatch match;
  if (!std::regex_search(object, match, pattern)) {
    return false;
  }
  *value = std::stod(match[1].str());
  return true;
}

bool readStringField(
    const std::string& object,
    const char* key,
    std::string* value) {
  const std::regex pattern(
      std::string("\"") + key + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch match;
  if (!std::regex_search(object, match, pattern)) {
    return false;
  }
  *value = match[1].str();
  return true;
}

std::string blockJson(const BlockProvenance& block) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\"block_id\":" << block.block_id
      << ",\"level\":" << block.level
      << ",\"is_contact_band_block\":"
      << boolJson(block.is_contact_band_block)
      << ",\"is_far_field_block\":" << boolJson(block.is_far_field_block)
      << ",\"is_coverage_promoted\":"
      << boolJson(block.is_coverage_promoted)
      << ",\"has_exact_contact_cells\":"
      << boolJson(block.has_exact_contact_cells)
      << ",\"exact_node_count\":" << block.exact_node_count
      << ",\"predicted_node_count\":" << block.predicted_node_count
      << ",\"logical_node_count\":" << block.logical_node_count
      << ",\"sampling_mode\":" << quote(block.sampling_mode)
      << ",\"marker_mode\":" << quote(block.marker_mode)
      << ",\"promotion_reason\":" << quote(block.promotion_reason)
      << ",\"contact_band_width\":" << block.contact_band_width
      << ",\"coverage_near_band\":" << block.coverage_near_band << "}";
  return out.str();
}

}  // namespace

const BlockProvenance* BlockProvenanceSet::find(int block_id) const {
  const auto iter = std::find_if(
      blocks.begin(),
      blocks.end(),
      [block_id](const BlockProvenance& item) {
        return item.block_id == block_id;
      });
  return iter == blocks.end() ? nullptr : &*iter;
}

BlockProvenance* BlockProvenanceSet::find(int block_id) {
  const auto iter = std::find_if(
      blocks.begin(),
      blocks.end(),
      [block_id](const BlockProvenance& item) {
        return item.block_id == block_id;
      });
  return iter == blocks.end() ? nullptr : &*iter;
}

BlockProvenanceSummary BlockProvenanceSet::summary() const {
  BlockProvenanceSummary out;
  out.block_count = blocks.size();
  for (const BlockProvenance& block : blocks) {
    if (block.is_contact_band_block) {
      ++out.contact_band_block_count;
    }
    if (block.is_far_field_block) {
      ++out.far_field_block_count;
    }
    if (block.is_coverage_promoted) {
      ++out.coverage_promoted_block_count;
    }
    out.exact_node_total += block.exact_node_count;
    out.predicted_node_total += block.predicted_node_count;
    out.logical_node_total += block.logical_node_count;
  }
  return out;
}

std::filesystem::path BlockProvenanceIO::defaultSidecarPath(
    const std::filesystem::path& sdf_path) {
  std::filesystem::path out = sdf_path;
  out += ".provenance.json";
  return out;
}

BlockProvenanceSet BlockProvenanceIO::fromAdaptiveModel(
    const AdaptiveBlockSDFModel& model,
    const AdaptiveBlockSDFBuildReport& build_report,
    const std::string& sampling_mode,
    double contact_band_width,
    double coverage_near_band) {
  BlockProvenanceSet out;
  const auto stats = statsByBlockId(build_report);
  const std::string marker_mode = build_report.contact_band_sampling.marker_mode;
  for (const AdaptiveSDFBlock& block : model.blockSet().blocks) {
    out.blocks.push_back(makeBaseProvenance(
        block.block_id,
        block.level,
        block.near_surface,
        logicalNodeCount(block.nx, block.ny, block.nz),
        stats,
        sampling_mode,
        marker_mode,
        contact_band_width,
        coverage_near_band));
  }
  return out;
}

BlockProvenanceSet BlockProvenanceIO::fromCompressedModel(
    const CompressedAdaptiveBlockSDFModel& model,
    const AdaptiveBlockSDFBuildReport& build_report,
    const std::string& sampling_mode,
    double contact_band_width,
    double coverage_near_band) {
  BlockProvenanceSet out;
  const auto stats = statsByBlockId(build_report);
  const std::string marker_mode = build_report.contact_band_sampling.marker_mode;
  for (const CompressedSDFBlock& block : model.compressedBlockSet().blocks) {
    out.blocks.push_back(makeBaseProvenance(
        block.block_id,
        block.level,
        block.near_surface,
        logicalNodeCount(block.nx, block.ny, block.nz),
        stats,
        sampling_mode,
        marker_mode,
        contact_band_width,
        coverage_near_band));
  }
  return out;
}

void BlockProvenanceIO::markCoveragePromoted(
    BlockProvenanceSet* provenance,
    const std::vector<int>& block_ids,
    const std::string& reason,
    double coverage_near_band) {
  if (provenance == nullptr) {
    return;
  }
  for (int block_id : block_ids) {
    BlockProvenance* block = provenance->find(block_id);
    if (block == nullptr) {
      continue;
    }
    block->is_contact_band_block = true;
    block->is_far_field_block = false;
    block->is_coverage_promoted = true;
    block->has_exact_contact_cells = true;
    block->promotion_reason = reason;
    block->coverage_near_band = coverage_near_band;
  }
}

bool BlockProvenanceIO::writeJson(
    const std::filesystem::path& path,
    const BlockProvenanceSet& provenance,
    std::string* error) {
  try {
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
      if (error != nullptr) {
        *error = "could not open provenance JSON for writing";
      }
      return false;
    }
    file << std::setprecision(17);
    file << "{\n";
    file << "  \"schema_id\": " << quote(provenance.schema_id) << ",\n";
    file << "  \"provenance_format\": "
         << quote(provenance.provenance_format) << ",\n";
    file << "  \"sdf_path\": " << quote(provenance.sdf_path.string())
         << ",\n";
    file << "  \"summary\": " << summaryJson(provenance) << ",\n";
    file << "  \"blocks\": [\n";
    for (std::size_t i = 0; i < provenance.blocks.size(); ++i) {
      file << "    " << blockJson(provenance.blocks[i]);
      if (i + 1 < provenance.blocks.size()) {
        file << ",";
      }
      file << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    return static_cast<bool>(file);
  } catch (const std::exception& exc) {
    if (error != nullptr) {
      *error = exc.what();
    }
    return false;
  }
}

bool BlockProvenanceIO::readJson(
    const std::filesystem::path& path,
    BlockProvenanceSet* provenance,
    std::string* error) {
  if (provenance == nullptr) {
    if (error != nullptr) {
      *error = "null provenance output";
    }
    return false;
  }
  std::ifstream file(path);
  if (!file) {
    if (error != nullptr) {
      *error = "could not open provenance JSON";
    }
    return false;
  }
  const std::string text(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  BlockProvenanceSet out;
  readStringField(text, "schema_id", &out.schema_id);
  readStringField(text, "provenance_format", &out.provenance_format);
  std::string sdf;
  if (readStringField(text, "sdf_path", &sdf)) {
    out.sdf_path = sdf;
  }

  const std::regex object_pattern("\\{[^\\{\\}]*\"block_id\"[^\\{\\}]*\\}");
  for (std::sregex_iterator it(text.begin(), text.end(), object_pattern), end;
       it != end;
       ++it) {
    const std::string object = it->str();
    BlockProvenance block;
    if (!readIntField(object, "block_id", &block.block_id)) {
      continue;
    }
    readIntField(object, "level", &block.level);
    readBoolField(object, "is_contact_band_block", &block.is_contact_band_block);
    readBoolField(object, "is_far_field_block", &block.is_far_field_block);
    readBoolField(object, "is_coverage_promoted", &block.is_coverage_promoted);
    readBoolField(object, "has_exact_contact_cells", &block.has_exact_contact_cells);
    readSizeField(object, "exact_node_count", &block.exact_node_count);
    readSizeField(object, "predicted_node_count", &block.predicted_node_count);
    readSizeField(object, "logical_node_count", &block.logical_node_count);
    readStringField(object, "sampling_mode", &block.sampling_mode);
    readStringField(object, "marker_mode", &block.marker_mode);
    readStringField(object, "promotion_reason", &block.promotion_reason);
    readDoubleField(object, "contact_band_width", &block.contact_band_width);
    readDoubleField(object, "coverage_near_band", &block.coverage_near_band);
    out.blocks.push_back(std::move(block));
  }
  *provenance = std::move(out);
  return true;
}

bool BlockProvenanceIO::readSidecarForSDF(
    const std::filesystem::path& sdf_path,
    BlockProvenanceSet* provenance,
    std::string* error) {
  const std::filesystem::path sidecar = defaultSidecarPath(sdf_path);
  if (!std::filesystem::exists(sidecar)) {
    if (error != nullptr) {
      *error = "provenance sidecar not found: " + sidecar.string();
    }
    return false;
  }
  return readJson(sidecar, provenance, error);
}

std::string BlockProvenanceIO::summaryJson(
    const BlockProvenanceSet& provenance) {
  const BlockProvenanceSummary summary = provenance.summary();
  return "{\"provenance_present\":true,\"provenance_format\":" +
         quote(provenance.provenance_format) + ",\"block_count\":" +
         JsonContractWriter::integer(summary.block_count) +
         ",\"contact_band_block_count\":" +
         JsonContractWriter::integer(summary.contact_band_block_count) +
         ",\"far_field_block_count\":" +
         JsonContractWriter::integer(summary.far_field_block_count) +
         ",\"coverage_promoted_block_count\":" +
         JsonContractWriter::integer(summary.coverage_promoted_block_count) +
         ",\"exact_node_total\":" +
         JsonContractWriter::integer(summary.exact_node_total) +
         ",\"predicted_node_total\":" +
         JsonContractWriter::integer(summary.predicted_node_total) +
         ",\"logical_node_total\":" +
         JsonContractWriter::integer(summary.logical_node_total) + "}";
}

}  // namespace adasdf
