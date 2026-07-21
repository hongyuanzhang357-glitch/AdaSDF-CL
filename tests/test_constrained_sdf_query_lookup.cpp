#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <future>
#include <iostream>
#include <vector>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

adasdf::ConstrainedAdaptiveOctreeNode makeNode(
    int id,
    int parent,
    int level,
    const adasdf::AABB& bounds,
    bool leaf) {
  adasdf::ConstrainedAdaptiveOctreeNode node;
  node.node_id = id;
  node.parent_id = parent;
  node.level = level;
  node.bounds = bounds;
  node.is_leaf = leaf;
  return node;
}

adasdf::ConstrainedSDFAssetBlock makeConstantBlock(
    int block_id,
    int node_id,
    int level,
    const adasdf::AABB& bounds,
    double value) {
  adasdf::ConstrainedSDFAssetBlock block;
  block.plan.block_id = block_id;
  block.plan.source_octree_node_id = node_id;
  block.plan.level = level;
  block.plan.bounds = bounds;
  block.plan.tensor_nx = 2;
  block.plan.tensor_ny = 2;
  block.plan.tensor_nz = 2;
  block.tensor.requested_method = adasdf::TensorCompressionMethod::Dense;
  block.tensor.method = adasdf::TensorCompressionMethod::Dense;
  block.tensor.nx = 2;
  block.tensor.ny = 2;
  block.tensor.nz = 2;
  block.tensor.dense.assign(8, value);
  block.tensor.original_bytes = 8 * sizeof(double);
  block.tensor.compressed_bytes = block.tensor.original_bytes;
  block.tensor.decoded_peak_bytes = block.tensor.original_bytes;
  block.tensor.dense_fallback = true;
  return block;
}

}  // namespace

int main() {
  const adasdf::AABB root_bounds{
      {-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}, true};
  const adasdf::AABB low_bounds{
      {-1.0, -1.0, -1.0}, {0.0, 1.0, 1.0}, true};
  const adasdf::AABB high_bounds{
      {0.0, -1.0, -1.0}, {1.0, 1.0, 1.0}, true};
  const adasdf::AABB high_low_bounds{
      {0.0, -1.0, -1.0}, {0.5, 1.0, 1.0}, true};
  const adasdf::AABB high_high_bounds{
      {0.5, -1.0, -1.0}, {1.0, 1.0, 1.0}, true};
  const adasdf::AABB low_bottom_bounds{
      {-1.0, -1.0, -1.0}, {0.0, 0.0, 1.0}, true};
  const adasdf::AABB low_top_bounds{
      {-1.0, 0.0, -1.0}, {0.0, 1.0, 1.0}, true};

  adasdf::ConstrainedSDFAsset asset;
  asset.octree.bounds = root_bounds;
  asset.octree.root_id = 0;
  asset.octree.nodes.push_back(makeNode(0, -1, 0, root_bounds, false));
  asset.octree.nodes.push_back(makeNode(1, 0, 1, low_bounds, true));
  asset.octree.nodes.push_back(makeNode(2, 0, 1, high_bounds, false));
  asset.octree.nodes.push_back(makeNode(3, 2, 2, high_low_bounds, true));
  asset.octree.nodes.push_back(makeNode(4, 2, 2, high_high_bounds, true));
  asset.octree.nodes[0].child_ids[0] = 1;
  asset.octree.nodes[0].child_ids[1] = 2;
  asset.octree.nodes[2].child_ids[0] = 3;
  asset.octree.nodes[2].child_ids[1] = 4;
  asset.blocks.push_back(
      makeConstantBlock(10, 1, 1, low_bottom_bounds, 10.0));
  asset.blocks.push_back(
      makeConstantBlock(11, 1, 1, low_top_bounds, 11.0));
  asset.blocks.push_back(makeConstantBlock(19, 2, 1, high_bounds, 19.0));
  asset.blocks.push_back(makeConstantBlock(20, 3, 2, high_low_bounds, 20.0));
  asset.blocks.push_back(makeConstantBlock(21, 4, 2, high_high_bounds, 21.0));

  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "constrained_sdf_query_lookup";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / "mixed_level.sdfbin";
  adasdf::ConstrainedSDFBinReport report;
  if (!adasdf::ConstrainedSDFBin::write(path, asset, &report) ||
      !report.success) {
    std::cerr << report.error_message << "\n";
    return 1;
  }
  const auto model = adasdf::SDFBinReader::read(path);
  const auto decoded_model = adasdf::ConstrainedSDFModel::load(
      path, adasdf::ConstrainedSDFQueryStorage::DecodedCache);
  const std::vector<std::pair<adasdf::Vector3, double>> cases = {
      {{-0.25, -0.5, 0.0}, 10.0},
      {{-0.25, 0.0, 0.0}, 10.0},
      {{-0.25, 0.5, 0.0}, 11.0},
      {{0.0, 0.0, 0.0}, 20.0},
      {{0.25, 0.0, 0.0}, 20.0},
      {{0.5, 0.0, 0.0}, 20.0},
      {{0.75, 0.0, 0.0}, 21.0}};
  for (const auto& item : cases) {
    if (std::abs(model->sampleDistance(item.first) - item.second) > 1.0e-12) {
      std::cerr << "mixed-level block lookup changed boundary precedence\n";
      return 1;
    }
    if (decoded_model->sampleDistance(item.first) !=
        model->sampleDistance(item.first)) {
      std::cerr << "decoded cache changed constrained SDF query output\n";
      return 1;
    }
  }

  std::vector<std::future<bool>> workers;
  for (int worker = 0; worker < 8; ++worker) {
    workers.push_back(std::async(std::launch::async, [model, cases]() {
      for (int repeat = 0; repeat < 1000; ++repeat) {
        for (const auto& item : cases) {
          if (model->sampleDistance(item.first) != item.second) {
            return false;
          }
        }
      }
      return true;
    }));
  }
  for (auto& worker : workers) {
    if (!worker.get()) {
      std::cerr << "concurrent constrained SDF cache query was unstable\n";
      return 1;
    }
  }
  std::filesystem::remove_all(dir);
  return 0;
}
