#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/compression/AdaptiveTensorCompressor.h"
#include "adasdf/generation/ConstrainedAdaptiveOctree.h"
#include "adasdf/generation/ConstrainedBlockPartitioner.h"

namespace adasdf {

inline constexpr std::uint32_t kConstrainedSDFBinFormatVersion = 1;

struct ConstrainedSDFAssetBlock {
  ConstrainedSDFBlockPlan plan;
  CompressedTensor3D tensor;
};

struct ConstrainedSDFAsset {
  ConstrainedAdaptiveOctree octree;
  std::vector<ConstrainedSDFAssetBlock> blocks;
  double max_zero_surface_abs_error = 0.0;
  bool zero_surface_error_is_strict_bound = false;
};

struct ConstrainedSDFBinBlockRecord {
  int block_id = -1;
  int source_octree_node_id = -1;
  int level = 0;
  AABB bounds;
  int nx = 0;
  int ny = 0;
  int nz = 0;
  TensorCompressionMethod method = TensorCompressionMethod::Dense;
  int rank_x = 0;
  int rank_y = 0;
  int rank_z = 0;
  double tolerance = 0.0;
  double actual_max_abs_error = 0.0;
  std::uint64_t original_bytes = 0;
  std::uint64_t compressed_bytes = 0;
  std::uint64_t decoded_peak_bytes = 0;
  std::uint64_t payload_offset = 0;
  std::uint64_t payload_bytes = 0;
  std::uint64_t payload_checksum = 0;
};

struct ConstrainedSDFBinReport {
  bool success = false;
  std::string error_message;
  std::uint32_t format_version = 0;
  std::uint64_t file_bytes = 0;
  std::uint64_t file_checksum = 0;
  std::size_t octree_node_count = 0;
  std::size_t block_count = 0;
  bool checksum_verified = false;
};

class ConstrainedSDFBinReader {
 public:
  static ConstrainedSDFBinReader open(
      const std::filesystem::path& path,
      ConstrainedSDFBinReport* report = nullptr);

  bool valid() const { return valid_; }
  const std::filesystem::path& path() const { return path_; }
  const AABB& bounds() const { return bounds_; }
  double maxZeroSurfaceAbsError() const { return max_zero_surface_abs_error_; }
  bool zeroSurfaceErrorIsStrictBound() const {
    return zero_surface_error_is_strict_bound_;
  }
  const std::vector<ConstrainedAdaptiveOctreeNode>& octreeNodes() const {
    return octree_nodes_;
  }
  const std::vector<ConstrainedSDFBinBlockRecord>& blocks() const {
    return blocks_;
  }

  bool loadBlock(
      std::size_t block_index,
      CompressedTensor3D* tensor,
      std::string* error_message = nullptr) const;

 private:
  std::filesystem::path path_;
  AABB bounds_;
  double max_zero_surface_abs_error_ = 0.0;
  bool zero_surface_error_is_strict_bound_ = false;
  std::vector<ConstrainedAdaptiveOctreeNode> octree_nodes_;
  std::vector<ConstrainedSDFBinBlockRecord> blocks_;
  bool valid_ = false;
};

class ConstrainedSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);

  static bool write(
      const std::filesystem::path& path,
      const ConstrainedSDFAsset& asset,
      ConstrainedSDFBinReport* report = nullptr);
};

}  // namespace adasdf
