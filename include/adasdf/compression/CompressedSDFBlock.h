#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

enum class BlockCompressionMethod {
  DenseFallback,
  MatrixSVD,
  TuckerPreview
};

const char* blockCompressionMethodName(BlockCompressionMethod method);
BlockCompressionMethod blockCompressionMethodFromName(const std::string& name);

struct MatrixSVDBlockData {
  int rows = 0;
  int cols = 0;
  int rank = 0;

  int nx = 0;
  int ny = 0;
  int nz = 0;

  std::vector<double> U;
  std::vector<double> S;
  std::vector<double> Vt;
};

struct CompressedSDFBlock {
  int block_id = -1;
  int source_block_id = -1;
  int octree_node_id = -1;
  int level = 0;

  AABB bounds;

  int nx = 0;
  int ny = 0;
  int nz = 0;

  Vector3 origin;
  Vector3 spacing;

  bool near_surface = false;
  bool signed_distance = true;

  BlockCompressionMethod method = BlockCompressionMethod::DenseFallback;

  std::vector<double> dense_phi;
  MatrixSVDBlockData svd;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_abs_error = 0.0;

  std::size_t original_bytes = 0;
  std::size_t compressed_bytes = 0;

  bool compression_success = false;
  std::string compression_note;
};

struct CompressedAdaptiveBlockSDF {
  std::vector<CompressedSDFBlock> blocks;
  AABB global_bounds;
  bool signed_distance = true;

  std::size_t blockCount() const;
  std::size_t originalMemoryBytes() const;
  std::size_t compressedMemoryBytes() const;
  double compressionRatio() const;
};

double compressedBlockGridValue(
    const CompressedSDFBlock& block,
    int ix,
    int iy,
    int iz);

}  // namespace adasdf
