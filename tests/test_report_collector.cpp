#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  std::vector<std::filesystem::path> reports;
  for (int i = 0; i < 2; ++i) {
    adasdf::RunManifest manifest =
        adasdf::makeRunManifest("collector", "case" + std::to_string(i), {}, {});
    manifest.status = "ok";
    manifest.success = true;
    manifest.start_time_utc = "2026-07-04T00:00:00Z";
    manifest.end_time_utc = "2026-07-04T00:00:00Z";
    manifest.metrics["triangle_count"] = static_cast<double>(i + 1);
    const std::filesystem::path path =
        temp / ("strict_report_" + std::to_string(i) + ".json");
    std::string error;
    if (!adasdf::StrictJsonWriter::writeFile(path, manifest, &error)) {
      std::cerr << error << "\n";
      return 1;
    }
    reports.push_back(path);
  }
  const std::filesystem::path list = temp / "strict_report_list.txt";
  {
    std::ofstream out(list);
    for (const auto& report : reports) {
      out << report.string() << "\n";
    }
  }
  const std::filesystem::path csv = temp / "summary.csv";
  std::string error;
  if (!adasdf::ReportCollector::collectRunSummary(list, csv, &error)) {
    std::cerr << error << "\n";
    return 1;
  }
  std::ifstream in(csv);
  std::string text((std::istreambuf_iterator<char>(in)), {});
  if (text.find("schema_version,adasdf_version") == std::string::npos ||
      text.find("case1") == std::string::npos) {
    std::cerr << "report collector summary missing expected data\n";
    return 1;
  }
  std::cout << "report collector passed\n";
  return 0;
}

