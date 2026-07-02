#pragma once

#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>

namespace active_block_tests {

inline double phiAt(const adasdf::Vector3& p) {
  return p.x - 0.3;
}

inline adasdf::CompressedSDFBlock makeDenseFallbackBlock(
    int block_id,
    double min_x,
    double max_x) {
  adasdf::CompressedSDFBlock block;
  block.block_id = block_id;
  block.source_block_id = block_id;
  block.level = 1;
  block.bounds = {{min_x, 0.0, 0.0}, {max_x, 1.0, 1.0}, true};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.origin = block.bounds.min;
  block.spacing = {(max_x - min_x) / 2.0, 0.5, 0.5};
  block.method = adasdf::BlockCompressionMethod::DenseFallback;
  block.dense_phi.resize(27);
  for (int k = 0; k < block.nz; ++k) {
    for (int j = 0; j < block.ny; ++j) {
      for (int i = 0; i < block.nx; ++i) {
        const adasdf::Vector3 p{
            block.origin.x + block.spacing.x * i,
            block.origin.y + block.spacing.y * j,
            block.origin.z + block.spacing.z * k};
        const std::size_t index =
            static_cast<std::size_t>(i) +
            static_cast<std::size_t>(block.nx) *
                (static_cast<std::size_t>(j) +
                 static_cast<std::size_t>(block.ny) *
                     static_cast<std::size_t>(k));
        block.dense_phi[index] = phiAt(p);
      }
    }
  }
  block.original_bytes = block.dense_phi.size() * sizeof(double);
  block.compressed_bytes = block.original_bytes;
  block.compression_success = true;
  return block;
}

inline adasdf::CompressedAdaptiveBlockSDFModel makeCompressedModel() {
  adasdf::CompressedAdaptiveBlockSDF compressed;
  compressed.blocks.push_back(makeDenseFallbackBlock(0, 0.0, 0.5));
  compressed.blocks.push_back(makeDenseFallbackBlock(1, 0.5, 1.0));
  compressed.global_bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  compressed.signed_distance = true;
  return adasdf::CompressedAdaptiveBlockSDFModel(std::move(compressed));
}

inline adasdf::CollisionSampleSet makeSamples() {
  adasdf::CollisionSampleSet samples;
  adasdf::CollisionSample inside;
  inside.sample_id = 0;
  inside.position = {0.2, 0.5, 0.5};
  inside.label = "inside";
  samples.add(inside);

  adasdf::CollisionSample outside;
  outside.sample_id = 1;
  outside.position = {0.8, 0.5, 0.5};
  outside.label = "outside";
  samples.add(outside);
  return samples;
}

inline std::filesystem::path tempDir() {
  std::filesystem::path path = ADASDF_CL_TEST_TEMP_DIR;
  if (path.empty()) {
    path = std::filesystem::temp_directory_path() / "adasdf_active_block_tests";
  }
  std::filesystem::create_directories(path);
  return path;
}

}  // namespace active_block_tests
