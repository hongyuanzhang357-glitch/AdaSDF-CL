#pragma once

#include <map>
#include <string>
#include <vector>

namespace adasdf {

struct StrictReportRecord {
  std::map<std::string, std::string> fields;
  std::map<std::string, std::string> metrics;
};

const char* strictReportSchemaVersion();
const std::vector<std::string>& strictReportRequiredFields();
const std::vector<std::string>& strictReportOptionalMetricFields();
const std::vector<std::string>& strictReportSummaryCsvHeader();

bool isStrictReportRequiredField(const std::string& field);
bool isStrictReportOptionalMetricField(const std::string& field);

}  // namespace adasdf

