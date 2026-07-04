#include "adasdf/report/StrictCsvWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {

std::string StrictCsvWriter::csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string out = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      out += "\"\"";
    } else {
      out.push_back(ch);
    }
  }
  out.push_back('"');
  return out;
}

std::string StrictCsvWriter::headerLine() {
  std::ostringstream out;
  const auto& header = strictReportSummaryCsvHeader();
  for (std::size_t i = 0; i < header.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << header[i];
  }
  return out.str();
}

std::string StrictCsvWriter::rowLine(const StrictReportRecord& record) {
  std::ostringstream out;
  const auto& header = strictReportSummaryCsvHeader();
  for (std::size_t i = 0; i < header.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    const std::string& name = header[i];
    auto field = record.fields.find(name);
    if (field != record.fields.end()) {
      out << csvField(field->second);
      continue;
    }
    auto metric = record.metrics.find(name);
    if (metric != record.metrics.end()) {
      out << csvField(metric->second);
    }
  }
  return out.str();
}

bool StrictCsvWriter::writeSummaryCSV(
    const std::filesystem::path& path,
    const std::vector<StrictReportRecord>& records,
    std::string* error_message) {
  try {
    if (!path.parent_path().empty()) {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
      if (error_message) {
        *error_message = "failed to open summary CSV: " + path.string();
      }
      return false;
    }
    file << headerLine() << "\n";
    for (const StrictReportRecord& record : records) {
      file << rowLine(record) << "\n";
    }
    return true;
  } catch (const std::exception& exc) {
    if (error_message) {
      *error_message = exc.what();
    }
    return false;
  }
}

}  // namespace adasdf

