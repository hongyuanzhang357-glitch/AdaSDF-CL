#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/compression/LowRankBlock.h"

namespace adasdf {

struct ContactOnlySDFBinHeader {
  std::uint32_t magic = 0x4144434c;  // "ADCL"
  std::uint32_t version = 1;
  std::string model_unit = "meter";
  AABB global_bounds;
  std::uint64_t block_count = 0;
  Scalar near_surface_error = 0.0;
  Scalar max_reconstruction_error = 0.0;
  bool has_checksum = false;
};

struct ContactOnlySDFBin {
  ContactOnlySDFBinHeader header;
  std::vector<LowRankBlock> blocks;

  // TODO: implement after the existing .sdfbin reader/writer is stabilized.
  static ContactOnlySDFBin read(const std::filesystem::path& path);
  void write(const std::filesystem::path& path) const;
};

}  // namespace adasdf
