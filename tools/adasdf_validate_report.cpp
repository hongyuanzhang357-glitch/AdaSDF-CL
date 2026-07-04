#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout << "Usage: adasdf_validate_report report.json\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
    usage();
    return argc == 1 ? 0 : 0;
  }
  if (argc != 2) {
    usage();
    return 1;
  }

  const std::filesystem::path report = argv[1];
  const adasdf::ReportValidationResult result =
      adasdf::ReportValidator::validateFile(report);
  if (!result.readable) {
    std::cerr << "adasdf_validate_report: file/read failed\n";
    for (const std::string& error : result.errors) {
      std::cerr << "- " << error << "\n";
    }
    return 1;
  }
  if (!result.valid) {
    std::cerr << "adasdf_validate_report: schema invalid\n";
    for (const std::string& error : result.errors) {
      std::cerr << "- " << error << "\n";
    }
    return 2;
  }
  std::cout << "Strict report valid: " << report.string() << "\n";
  return 0;
}

