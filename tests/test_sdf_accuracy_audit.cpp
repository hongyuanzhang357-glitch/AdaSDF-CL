#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    if (!read.success) {
      std::cerr << "failed to read fixture\n";
      return 1;
    }
    adasdf::DenseSDFBuildOptions build_options;
    build_options.resolution = 32;
    build_options.padding = 0.05;
    build_options.verbose = false;
    adasdf::DenseSDFBuildReport build_report;
    const auto model =
        adasdf::DenseSDFBuilder::fromMesh(read.mesh, build_options, &build_report);
    if (!model || !build_report.success || !model->queryBackendAvailable()) {
      std::cerr << "dense model build failed\n";
      return 1;
    }
    adasdf::NearSurfaceSampleOptions sample_options;
    sample_options.surface_sample_count = 12;
    sample_options.offsets = {-0.01, 0.0, 0.01};
    const adasdf::NearSurfaceSampleSet samples =
        adasdf::NearSurfaceSampleGenerator::generate(read.mesh, sample_options);
    adasdf::SDFAccuracyAuditOptions audit_options;
    audit_options.input_stl = fixture;
    audit_options.case_id = "cube_unit_audit";
    audit_options.near_band = 0.05;
    audit_options.normal_audit = true;
    const adasdf::SDFAccuracyAuditResult result =
        adasdf::SDFAccuracyAudit::run(
            *model,
            read.mesh,
            samples,
            audit_options);
    if (result.sample_count_total != samples.samples.size() ||
        result.near_surface_sample_count == 0 ||
        result.sdf_query_count == 0 ||
        result.exact_reference_query_count == 0 ||
        result.query_failed_count != 0 ||
        !std::isfinite(result.p95_abs_error) ||
        result.normal_check_count == 0) {
      std::cerr << "audit result missing expected metrics\n";
      return 1;
    }
    const std::string json = adasdf::SDFAccuracyReportWriter::toJson(result);
    const std::string md = adasdf::SDFAccuracyReportWriter::toMarkdown(result);
    if (json.find("adasdf.sdf_accuracy_audit.v1") == std::string::npos ||
        json.find("near_surface_sample_count") == std::string::npos ||
        md.find("SDF Near-Surface Accuracy Audit") == std::string::npos) {
      std::cerr << "audit writers missing expected content\n";
      return 1;
    }
    std::cout << "sdf accuracy audit passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_sdf_accuracy_audit failed: " << exc.what() << "\n";
    return 1;
  }
}
