#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  adasdf::ConstrainedSDFAsset asset;
  asset.octree.bounds = {{-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}, true};
  asset.octree.root_id = 0;
  adasdf::ConstrainedAdaptiveOctreeNode root;
  root.node_id = 0;
  root.bounds = asset.octree.bounds;
  asset.octree.nodes.push_back(root);
  asset.max_zero_surface_abs_error = 1.0e-4;
  asset.zero_surface_error_is_strict_bound = false;

  adasdf::ConstrainedSDFAssetBlock block;
  block.plan.block_id = 0;
  block.plan.source_octree_node_id = 0;
  block.plan.bounds = asset.octree.bounds;
  block.plan.tensor_nx = 3;
  block.plan.tensor_ny = 3;
  block.plan.tensor_nz = 3;
  std::vector<double> phi(27);
  for (std::size_t i = 0; i < phi.size(); ++i) {
    phi[i] = static_cast<double>(i) * 0.01 - 0.1;
  }
  adasdf::AdaptiveTensorCompressionOptions compression;
  compression.method = adasdf::TensorCompressionMethod::TensorTrain;
  compression.max_rank = 3;
  compression.max_abs_error = 1.0e-10;
  compression.near_zero_max_abs_error = 1.0e-10;
  block.tensor = adasdf::AdaptiveTensorCompressor::compress(
      phi, 3, 3, 3, compression);
  asset.blocks.push_back(block);

  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "constrained_sdfbin_roundtrip";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / "model.sdfbin";
  adasdf::ConstrainedSDFBinReport write_report;
  if (!adasdf::ConstrainedSDFBin::write(path, asset, &write_report) ||
      !write_report.success || write_report.file_bytes == 0) {
    std::cerr << write_report.error_message << "\n";
    return 1;
  }
  adasdf::ConstrainedSDFBinReport read_report;
  const auto reader = adasdf::ConstrainedSDFBinReader::open(path, &read_report);
  if (!reader.valid() || !read_report.success ||
      !read_report.checksum_verified || reader.blocks().size() != 1 ||
      reader.octreeNodes().size() != 1) {
    std::cerr << read_report.error_message << "\n";
    return 1;
  }
  adasdf::CompressedTensor3D loaded;
  std::string error;
  if (!reader.loadBlock(0, &loaded, &error)) {
    std::cerr << error << "\n";
    return 1;
  }
  const auto decoded = adasdf::AdaptiveTensorCompressor::decode(loaded);
  if (decoded.size() != phi.size()) {
    return 1;
  }
  for (std::size_t i = 0; i < phi.size(); ++i) {
    if (std::abs(decoded[i] - phi[i]) > 1.0e-10) {
      std::cerr << "serialized tensor round trip changed phi\n";
      return 1;
    }
  }
  const std::shared_ptr<adasdf::SDFModel> model =
      adasdf::SDFBinReader::read(path);
  if (!model || !model->isValid() || !model->queryBackendAvailable() ||
      std::abs(model->sampleDistance({0.0, 0.0, 0.0}) - phi[13]) > 1.0e-10) {
    std::cerr << "generic SDFBinReader constrained query failed\n";
    return 1;
  }

  std::fstream corrupt(path, std::ios::binary | std::ios::in | std::ios::out);
  corrupt.seekg(24);
  char byte = 0;
  corrupt.read(&byte, 1);
  byte ^= 0x5a;
  corrupt.seekp(24);
  corrupt.write(&byte, 1);
  corrupt.close();
  adasdf::ConstrainedSDFBinReport corrupt_report;
  const auto rejected =
      adasdf::ConstrainedSDFBinReader::open(path, &corrupt_report);
  if (rejected.valid() || corrupt_report.error_message.find("checksum") ==
          std::string::npos) {
    std::cerr << "corrupt constrained SDF file was not rejected\n";
    return 1;
  }
  std::filesystem::remove_all(dir);
  return 0;
}
