#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::ContactCandidate candidate;
  candidate.sample_id = 1;
  candidate.penetration_depth = 0.2;
  candidate.has_normal = true;
  candidate.normal = {1.0, 0.0, 0.0};
  auto result = adasdf::ContactStabilizer::stabilize({candidate});
  result.stats.warnings.push_back("test warning");
  const std::string markdown =
      adasdf::ContactStabilizationReportWriter::toMarkdown(result);
  const std::string json = adasdf::ContactStabilizationReportWriter::toJson(result);
  if (markdown.find("Patches") == std::string::npos ||
      markdown.find("test warning") == std::string::npos ||
      json.find("output_contact_count") == std::string::npos) {
    std::cerr << "contact stabilization report failed\n";
    return 1;
  }
  std::cout << "contact stabilization report writer passed\n";
  return 0;
}
