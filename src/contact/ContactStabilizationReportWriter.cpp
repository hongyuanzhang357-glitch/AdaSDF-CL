#include "adasdf/contact/ContactStabilizationReportWriter.h"

#include <sstream>

#include "adasdf/contact/SolverContact.h"

namespace adasdf {
namespace {

std::string boolText(bool value) {
  return value ? "true" : "false";
}

}  // namespace

std::string ContactStabilizationReportWriter::toMarkdown(
    const ContactStabilizationResult& result) {
  std::ostringstream out;
  out << "# Contact Stabilization Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- Input candidates: " << result.stats.input_candidate_count << "\n";
  out << "- After duplicate removal: "
      << result.stats.after_duplicate_removal_count << "\n";
  out << "- After threshold: " << result.stats.after_threshold_count << "\n";
  out << "- Patches: " << result.stats.patch_count << "\n";
  out << "- Output solver contacts: "
      << result.stats.output_contact_count << "\n";
  out << "- Max contacts total: "
      << result.stats.budget_stats.after_total_budget_count << "\n\n";

  if (!result.stats.warnings.empty()) {
    out << "## Warnings\n\n";
    for (const std::string& warning : result.stats.warnings) {
      out << "- " << warning << "\n";
    }
    out << "\n";
  }

  out << "## Patches\n\n";
  out << "| patch_id | members | representative_sample_id | max_penetration | mean_penetration |\n";
  out << "| --- | --- | --- | --- | --- |\n";
  for (const ContactPatch& patch : result.patches) {
    out << "| " << patch.patch_id
        << " | " << patch.members.size()
        << " | " << patch.representative_sample_id
        << " | " << patch.max_penetration_depth
        << " | " << patch.mean_penetration_depth << " |\n";
  }

  out << "\n## Final Contacts\n\n";
  out << "| sample_id | penetration_depth | object_id | link_id | group_id | label |\n";
  out << "| --- | --- | --- | --- | --- | --- |\n";
  for (const ContactCandidate& candidate : result.stabilized_candidates) {
    out << "| " << candidate.sample_id
        << " | " << candidate.penetration_depth
        << " | " << candidate.object_id
        << " | " << candidate.link_id
        << " | " << candidate.group_id
        << " | " << candidate.label << " |\n";
  }
  out << "\nLimitations: solver-ready contacts are not solver impulses, "
         "friction forces, Jacobian rows, or constraints.\n";
  return out.str();
}

std::string ContactStabilizationReportWriter::toJson(
    const ContactStabilizationResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"input_candidate_count\": "
      << result.stats.input_candidate_count << ",\n";
  out << "  \"after_duplicate_removal_count\": "
      << result.stats.after_duplicate_removal_count << ",\n";
  out << "  \"after_threshold_count\": "
      << result.stats.after_threshold_count << ",\n";
  out << "  \"patch_count\": " << result.stats.patch_count << ",\n";
  out << "  \"output_contact_count\": "
      << result.stats.output_contact_count << ",\n";
  out << "  \"limitations\": \"not a solver; no impulses or friction computed\",\n";
  out << "  \"warnings\": [";
  for (std::size_t i = 0; i < result.stats.warnings.size(); ++i) {
    out << "\"" << result.stats.warnings[i] << "\"";
    if (i + 1 < result.stats.warnings.size()) {
      out << ", ";
    }
  }
  out << "]\n";
  out << "}\n";
  return out.str();
}

}  // namespace adasdf
