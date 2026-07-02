#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "active_block_test_helpers.h"

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    const auto samples = active_block_tests::makeSamples();
    adasdf::ExpandedBlockCache cache;
    const auto result =
        adasdf::ActiveBlockQuery::query(model, samples, {0}, cache);
    const auto path = active_block_tests::tempDir() / "active_block_report.csv";
    std::string error;
    if (!adasdf::ActiveBlockReportWriter::writeQueryCSV(
            path.string(), result, &error)) {
      std::cerr << error << "\n";
      return 1;
    }
    const std::string csv = readFile(path);
    const std::string md =
        adasdf::ActiveBlockReportWriter::queryToMarkdown(result);
    if (csv.find("source") == std::string::npos ||
        csv.find("fallback") == std::string::npos ||
        md.find("Active Block Query Report") == std::string::npos) {
      std::cerr << "active block report writer failed\n";
      return 1;
    }
    std::cout << "active block report writer passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_report_writer failed: "
              << exc.what() << "\n";
    return 1;
  }
}
