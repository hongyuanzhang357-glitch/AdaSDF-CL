#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  const std::string header = adasdf::StrictCsvWriter::headerLine();
  const std::string expected_prefix =
      "schema_version,adasdf_version,git_commit,tool_name,case_id";
  if (header.rfind(expected_prefix, 0) != 0 ||
      header.find("broadphase_pair_count") == std::string::npos) {
    std::cerr << "strict CSV header is not fixed as expected\n";
    return 1;
  }
  adasdf::StrictReportRecord record;
  record.fields["schema_version"] = "adasdf.strict_report.v1";
  record.fields["tool_name"] = "tool,with,commas";
  record.metrics["triangle_count"] = "4";
  const std::string row = adasdf::StrictCsvWriter::rowLine(record);
  if (row.find("\"tool,with,commas\"") == std::string::npos) {
    std::cerr << "strict CSV escaping failed\n";
    return 1;
  }
  std::cout << "strict CSV writer passed\n";
  return 0;
}

