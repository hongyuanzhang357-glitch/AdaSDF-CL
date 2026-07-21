#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

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
      std::cerr << "failed to read cube fixture\n";
      return 1;
    }

    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    build_options.max_octree_level = 1;
    build_options.block_resolution = 5;
    build_options.padding = 0.05;
    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto base =
        adasdf::AdaptiveBlockSDFBuilder::fromMesh(
            read.mesh,
            build_options,
            &build_report);
    auto model =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(base);
    if (!model || !model->isValid()) {
      std::cerr << "adaptive model build failed\n";
      return 1;
    }

    const adasdf::Vector3 point{0.5, 0.5, 0.5};
    adasdf::SDFAccuracyAuditSample mismatch;
    mismatch.sample_id = 42;
    mismatch.point = point;
    mismatch.reference_phi = -0.5;
    mismatch.sdf_phi = 0.25;
    mismatch.abs_error = 0.75;
    mismatch.sign_mismatch = true;
    mismatch.reference_sign = -1;
    mismatch.sdf_sign = 1;
    mismatch.false_outside = true;
    mismatch.near_surface = true;
    mismatch.nearest_triangle_id = 0;
    const int block_index = model->findContainingBlock(point);
    if (block_index < 0) {
      std::cerr << "sample point did not map to an adaptive block\n";
      return 1;
    }
    mismatch.block_id =
        model->blockSet().blocks[static_cast<std::size_t>(block_index)]
            .block_id;

    adasdf::MismatchCellForensicsOptions options;
    options.case_id = "unit_mismatch_forensics";
    options.near_band = 0.1;
    options.local_subgrids = {2};
    options.max_cells = 8;
    const adasdf::MismatchCellForensicsResult result =
        adasdf::MismatchCellForensics::run(
            *model,
            read.mesh,
            std::vector<adasdf::SDFAccuracyAuditSample>{mismatch},
            options);
    if (result.sample_count != 1 || result.sign_mismatch_count != 1 ||
        result.false_outside_count != 1 || result.samples.empty()) {
      std::cerr << "forensics summary missing expected mismatch\n";
      return 1;
    }
    const adasdf::MismatchCellSampleDiagnostic& sample =
        result.samples.front();
    if (sample.block_id != mismatch.block_id ||
        sample.corner_pattern_category.empty() ||
        sample.zero_crossing_category.empty() ||
        sample.local_subgrid_results.find(2) ==
            sample.local_subgrid_results.end()) {
      std::cerr << "forensics sample missing expected detail\n";
      return 1;
    }
    const std::string json = adasdf::MismatchCellReportWriter::toJson(result);
    const std::string csv = adasdf::MismatchCellReportWriter::toCSV(result);
    const std::string md =
        adasdf::MismatchCellReportWriter::toMarkdown(result);
    if (json.find("adasdf.mismatch_cell_forensics.v1") ==
            std::string::npos ||
        json.find("zero_crossing_stats") == std::string::npos ||
        csv.find("zero_crossing_category") == std::string::npos ||
        md.find("Mismatch Cell Forensics") == std::string::npos) {
      std::cerr << "forensics writers missing expected content\n";
      return 1;
    }

    std::cout << "mismatch cell forensics passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mismatch_cell_forensics failed: "
              << exc.what() << "\n";
    return 1;
  }
}
