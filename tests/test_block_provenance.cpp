#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

adasdf::AdaptiveSDFBlock makeBlock() {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 7;
  block.level = 2;
  block.bounds.min = {0.0, 0.0, 0.0};
  block.bounds.max = {1.0, 1.0, 1.0};
  block.bounds.valid = true;
  block.origin = block.bounds.min;
  block.spacing = {0.5, 0.5, 0.5};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.near_surface = false;
  block.phi.assign(27, 1.0);
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.global_bounds.min = {0.0, 0.0, 0.0};
    blocks.global_bounds.max = {1.0, 1.0, 1.0};
    blocks.global_bounds.valid = true;
    blocks.blocks.push_back(makeBlock());
    adasdf::AdaptiveBlockSDFModel model(blocks);
    if (!model.isValid()) {
      std::cerr << "test model is invalid\n";
      return 1;
    }

    adasdf::AdaptiveBlockSDFBuildReport report;
    adasdf::AdaptiveTreeBlockSamplingStats stats;
    stats.block_id = 7;
    stats.level = 2;
    stats.far_field = true;
    stats.logical_node_count = 27;
    stats.predicted_node_count = 27;
    report.adaptive_tree_block_sampling_stats.push_back(stats);
    report.contact_band_sampling.marker_mode = "distance-aware";
    adasdf::BlockProvenanceSet provenance =
        adasdf::BlockProvenanceIO::fromAdaptiveModel(
            model,
            report,
            "contact-band",
            0.002,
            0.01);
    adasdf::BlockProvenanceIO::markCoveragePromoted(
        &provenance,
        {7},
        "unit-test",
        0.01);
    const adasdf::BlockProvenanceSummary summary = provenance.summary();
    if (summary.coverage_promoted_block_count != 1 ||
        summary.contact_band_block_count != 1 ||
        summary.far_field_block_count != 0) {
      std::cerr << "unexpected provenance summary\n";
      return 1;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const std::filesystem::path path = temp / "block_provenance.json";
    std::string error;
    if (!adasdf::BlockProvenanceIO::writeJson(path, provenance, &error)) {
      std::cerr << "failed to write provenance: " << error << "\n";
      return 1;
    }
    adasdf::BlockProvenanceSet read;
    if (!adasdf::BlockProvenanceIO::readJson(path, &read, &error)) {
      std::cerr << "failed to read provenance: " << error << "\n";
      return 1;
    }
    const adasdf::BlockProvenance* block = read.find(7);
    if (block == nullptr || !block->is_coverage_promoted ||
        block->promotion_reason != "unit-test") {
      std::cerr << "roundtrip provenance missing promoted block\n";
      return 1;
    }
    std::cout << "block provenance passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_block_provenance failed: " << exc.what() << "\n";
    return 1;
  }
}
