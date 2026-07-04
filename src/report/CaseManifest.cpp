#include "adasdf/report/CaseManifest.h"

namespace adasdf {

RunManifest runManifestFromCase(const CaseManifest& case_manifest) {
  RunManifest manifest = makeRunManifest(
      case_manifest.tool_name,
      case_manifest.case_id,
      case_manifest.input_path,
      case_manifest.output_path);
  manifest.parameters = case_manifest.parameters;
  return manifest;
}

}  // namespace adasdf

