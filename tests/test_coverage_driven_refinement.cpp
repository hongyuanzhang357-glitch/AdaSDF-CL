#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

namespace {

adasdf::TriangleMesh makeTriangleMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  mesh.triangles = {{0, 1, 2, 0}};
  return mesh;
}

adasdf::AdaptiveSDFBlock makeBlock() {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 3;
  block.level = 1;
  block.bounds.min = {-0.1, -0.1, -0.1};
  block.bounds.max = {1.1, 1.1, 0.1};
  block.bounds.valid = true;
  block.origin = block.bounds.min;
  block.spacing = {0.6, 0.6, 0.1};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.phi.assign(27, 5.0);
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.global_bounds.min = {-0.1, -0.1, -0.1};
    blocks.global_bounds.max = {1.1, 1.1, 0.1};
    blocks.global_bounds.valid = true;
    blocks.blocks.push_back(makeBlock());
    adasdf::AdaptiveBlockSDFModel model(blocks);
    adasdf::AdaptiveBlockSDFBuildReport report;
    adasdf::AdaptiveTreeBlockSamplingStats stats;
    stats.block_id = 3;
    stats.level = 1;
    stats.far_field = true;
    stats.logical_node_count = 27;
    stats.predicted_node_count = 27;
    report.adaptive_tree_block_sampling_stats.push_back(stats);
    adasdf::BlockProvenanceSet provenance =
        adasdf::BlockProvenanceIO::fromAdaptiveModel(
            model,
            report,
            "contact-band",
            0.001,
            0.01);

    adasdf::CoverageAuditResult audit;
    audit.missed_samples = 1;
    audit.missed_by_block_id[3] = 2;
    adasdf::CoverageMissRecord miss;
    miss.block_id = 3;
    miss.block_level = 1;
    miss.point = {0.2, 0.2, 0.001};
    audit.representative_misses.push_back(miss);

    adasdf::CoverageDrivenRefinementOptions options;
    options.mode = adasdf::CoveragePromotionMode::MissedCells;
    options.min_miss_count = 2;
    const adasdf::TriangleMesh mesh = makeTriangleMesh();
    const adasdf::CoverageDrivenRefinementResult result =
        adasdf::CoverageDrivenRefinement::promoteMisses(
            mesh,
            audit,
            options,
            &model,
            &report,
            &provenance);
    if (!result.applied || result.promoted_block_count != 1 ||
        result.promoted_cell_count == 0 || result.promoted_node_count == 0) {
      std::cerr << "coverage refinement did not promote expected cells\n";
      return 1;
    }
    const adasdf::BlockProvenance* block = provenance.find(3);
    if (block == nullptr || !block->is_coverage_promoted ||
        !block->has_exact_contact_cells || block->exact_node_count == 0) {
      std::cerr << "provenance did not record coverage promotion\n";
      return 1;
    }
    bool changed = false;
    for (double phi : model.blockSet().blocks.front().phi) {
      if (std::abs(phi - 5.0) > 1.0e-12) {
        changed = true;
        break;
      }
    }
    if (!changed) {
      std::cerr << "refinement did not resample any nodes\n";
      return 1;
    }
    std::cout << "coverage-driven refinement passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_coverage_driven_refinement failed: " << exc.what()
              << "\n";
    return 1;
  }
}
