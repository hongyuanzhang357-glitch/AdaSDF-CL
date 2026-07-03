#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  adasdf::ContactCandidate candidate;
  candidate.sample_id = 3;
  candidate.point = {1.0, 2.0, 3.0};
  candidate.normal = {0.0, 0.0, 1.0};
  candidate.has_normal = true;
  candidate.penetration_depth = 0.25;
  candidate.label = "export";
  auto contacts = adasdf::SolverContactBuilder::fromCandidates({candidate});
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto csv = temp / "solver_contact_exporter.csv";
  std::string error;
  if (!adasdf::SolverContactExporter::writeCSV(csv.string(), contacts, &error) ||
      readFile(csv).find("contact_id") == std::string::npos) {
    std::cerr << "solver contact CSV export failed\n";
    return 1;
  }
  if (adasdf::SolverContactExporter::toJson(contacts).find("contact_id") ==
      std::string::npos) {
    std::cerr << "solver contact JSON export failed\n";
    return 1;
  }
  if (adasdf::SolverContactExporter::toMarkdown(contacts).find("Contact count") ==
      std::string::npos) {
    std::cerr << "solver contact Markdown export failed\n";
    return 1;
  }
  std::cout << "solver contact exporter passed\n";
  return 0;
}
