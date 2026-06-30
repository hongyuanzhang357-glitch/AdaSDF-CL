#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

namespace {

bool auditModel(const adasdf::SDFModel& model) {
  adasdf::ExpansionOptions expansion_options;
  expansion_options.expansion = adasdf::QueryExpansionMode::Global;
  expansion_options.global_resolution = 16;
  const adasdf::ExpandedSDF expanded =
      adasdf::SDFExpander::expand(model, expansion_options);

  adasdf::ExpansionQualityOptions quality_options;
  quality_options.num_samples = 128;
  quality_options.near_surface_band = 0.05;
  const adasdf::ExpansionQualityReport report =
      adasdf::ExpansionQuality::compareAgainstDirect(
          model,
          expanded,
          quality_options);
  return report.num_finite_samples > 0 && report.fallback_count == 0;
}

}  // namespace

int main() {
  try {
    adasdf::DemoAdaptiveBuildRequest request;
    request.use_surrogate = false;
    const auto demo = adasdf::DemoAdaptiveSDFBuilder::build(request);
    if (!demo.model || !auditModel(*demo.model)) {
      std::cerr << "demo sampled ExpandedSDF bridge failed\n";
      return 1;
    }

    const std::filesystem::path fixture = ADASDF_CL_TEST_FIXTURE;
    if (fixture.empty()) {
      std::cout << "SKIP: no existing-core .sdfbin fixture discovered\n";
      return 0;
    }
    if (!adasdf::ExistingSDFBridge::existingCoreAvailable()) {
      std::cout << "SKIP: existing SDF core bridge is not available\n";
      return 0;
    }
    const auto existing = adasdf::SDFBinReader::read(fixture);
    if (!existing || !existing->queryBackendAvailable()) {
      std::cout << "SKIP: fixture has no direct query backend\n";
      return 0;
    }
    if (!auditModel(*existing)) {
      std::cerr << "existing-core sampled ExpandedSDF bridge failed\n";
      return 1;
    }

    std::cout << "existing-core expansion bridge passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_existing_core_expansion_bridge failed: "
              << exc.what() << "\n";
    return 1;
  }
}
