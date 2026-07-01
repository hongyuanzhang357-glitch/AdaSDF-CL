#include <adasdf/adasdf.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace {

void usage() {
  std::cout << "Usage: adasdf_info model.sdfbin\n";
}

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

void printAABB(const adasdf::AABB& aabb) {
  if (!aabb.valid) {
    std::cout << "AABB: invalid\n";
    return;
  }
  printVec("AABB min: ", aabb.min);
  printVec("AABB max: ", aabb.max);
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc != 2) {
    usage();
    return 2;
  }

  const std::filesystem::path path = argv[1];
  if (!std::filesystem::exists(path)) {
    std::cerr << "adasdf_info: file does not exist: " << path.string() << "\n";
    return 2;
  }

  try {
    const auto model = SDFBinReader::read(path);
    const SDFMetadata& metadata = model->metadata();
    const double memory_mb =
        static_cast<double>(model->memoryFootprintBytes()) / (1024.0 * 1024.0);

    std::cout << "AdaSDF-CL info\n";
    std::cout << "Library version: " << versionString() << "\n";
    std::cout << "Path: " << path.string() << "\n";
    std::cout << "Model name: " << model->debugName() << "\n";
    std::cout << "Valid: " << yesNo(model->isValid()) << "\n";
    if (!metadata.format_name.empty()) {
      std::cout << "Format: " << metadata.format_name << "\n";
    }
    if (const auto dense = std::dynamic_pointer_cast<DenseSDFModel>(model)) {
      const DenseSDFGrid& grid = dense->grid();
      std::cout << "DenseSDF resolution: " << grid.nx << " x " << grid.ny
                << " x " << grid.nz << "\n";
      printVec("DenseSDF origin: ", grid.origin);
      printVec("DenseSDF spacing: ", grid.spacing);
      std::cout << "DenseSDF signed: " << yesNo(grid.signed_distance) << "\n";
      std::cout << "Unit: " << grid.unit << "\n";
    }
    if (const auto adaptive_block =
            std::dynamic_pointer_cast<AdaptiveBlockSDFModel>(model)) {
      const AdaptiveSDFBlockSet& blocks = adaptive_block->blockSet();
      std::size_t near_surface_count = 0;
      int max_level_used = 0;
      int block_resolution = 0;
      for (const AdaptiveSDFBlock& block : blocks.blocks) {
        if (block.near_surface) {
          ++near_surface_count;
        }
        max_level_used = std::max(max_level_used, block.level);
        if (block_resolution == 0) {
          block_resolution = block.nx;
        }
      }
      std::cout << "AdaptiveBlockSDF block_count: "
                << blocks.blocks.size() << "\n";
      std::cout << "AdaptiveBlockSDF near_surface_block_count: "
                << near_surface_count << "\n";
      std::cout << "AdaptiveBlockSDF max_level_used: "
                << max_level_used << "\n";
      std::cout << "AdaptiveBlockSDF block_resolution: "
                << block_resolution << "\n";
      std::cout << "AdaptiveBlockSDF signed: "
                << yesNo(blocks.signed_distance) << "\n";
      std::cout << "AdaptiveBlockSDF storage: block-wise dense phi values\n";
      std::cout << "Low-rank compression: available in v1.7.0-alpha via "
                   "compressed block workflow\n";
    }
    if (const auto compressed =
            std::dynamic_pointer_cast<CompressedAdaptiveBlockSDFModel>(model)) {
      const CompressedAdaptiveBlockSDF& blocks = compressed->compressedBlockSet();
      std::size_t matrix_svd_count = 0;
      std::size_t dense_fallback_count = 0;
      std::size_t near_surface_count = 0;
      int max_level_used = 0;
      int min_rank = 0;
      int max_rank = 0;
      double max_error = 0.0;
      for (const CompressedSDFBlock& block : blocks.blocks) {
        if (block.method == BlockCompressionMethod::MatrixSVD) {
          ++matrix_svd_count;
          min_rank = min_rank == 0 ? block.svd.rank : std::min(min_rank, block.svd.rank);
          max_rank = std::max(max_rank, block.svd.rank);
        } else if (block.method == BlockCompressionMethod::DenseFallback) {
          ++dense_fallback_count;
        }
        if (block.near_surface) {
          ++near_surface_count;
        }
        max_level_used = std::max(max_level_used, block.level);
        max_error = std::max(max_error, block.max_abs_error);
      }
      std::cout << "CompressedBlockSDF block_count: "
                << blocks.blocks.size() << "\n";
      std::cout << "CompressedBlockSDF matrix_svd_block_count: "
                << matrix_svd_count << "\n";
      std::cout << "CompressedBlockSDF dense_fallback_block_count: "
                << dense_fallback_count << "\n";
      std::cout << "CompressedBlockSDF near_surface_block_count: "
                << near_surface_count << "\n";
      std::cout << "CompressedBlockSDF max_level_used: "
                << max_level_used << "\n";
      std::cout << "CompressedBlockSDF signed: "
                << yesNo(blocks.signed_distance) << "\n";
      std::cout << "CompressedBlockSDF original_memory_bytes: "
                << blocks.originalMemoryBytes() << "\n";
      std::cout << "CompressedBlockSDF compressed_memory_bytes: "
                << blocks.compressedMemoryBytes() << "\n";
      std::cout << "CompressedBlockSDF compression_ratio: "
                << blocks.compressionRatio() << "\n";
      std::cout << "CompressedBlockSDF rank_min: " << min_rank << "\n";
      std::cout << "CompressedBlockSDF rank_max: " << max_rank << "\n";
      std::cout << "CompressedBlockSDF max_abs_error: " << max_error << "\n";
      std::cout << "Tucker/HOSVD compression: not implemented in v1.7.0-alpha\n";
      std::cout << "GPU-native compressed query: planned\n";
    }
    if (const auto analytic = std::dynamic_pointer_cast<AnalyticSDFModel>(model)) {
      std::cout << "Shape: " << analytic->shapeName() << "\n";
      printVec("Center: ", analytic->center());
      printVec("Half extent: ", analytic->halfExtent());
      std::cout << "Unit: " << analytic->unit() << "\n";
    }
    if (const auto adaptive =
            std::dynamic_pointer_cast<DemoAdaptiveSDFModel>(model)) {
      const auto& description = adaptive->description();
      std::cout << "Shape: " << adaptive->shapeName() << "\n";
      printVec("Center: ", adaptive->center());
      printVec("Half extent: ", adaptive->halfExtent());
      std::cout << "Unit: " << adaptive->unit() << "\n";
      std::cout << "Target near-surface error: "
                << description.target_near_surface_error << "\n";
      std::cout << "Memory limit MB: " << description.memory_limit_mb << "\n";
      std::cout << "Block expand limit MB: "
                << description.block_expand_limit_mb << "\n";
      std::cout << "Surrogate: " << description.surrogate_id << "\n";
      std::cout << "Warning: " << description.warning << "\n";
      std::cout << "Demo octree nodes: "
                << adaptive->octreeNodes().size() << "\n";
      std::cout << "Demo adaptive blocks: "
                << adaptive->blocks().size() << "\n";
      if (!adaptive->blocks().empty()) {
        const DemoBlockInfo& block = adaptive->blocks().front();
        std::cout << "Block[0] resolution: " << block.resolution << "\n";
        std::cout << "Block[0] rank: " << block.rank << "\n";
        std::cout << "Block[0] estimated error: "
                  << block.estimated_error << "\n";
        std::cout << "Block[0] estimated memory KB: "
                  << block.estimated_memory_kb << "\n";
      }
    }
    std::cout << "Format version: " << metadata.format_version << "\n";
    std::cout << "Query backend: "
              << (metadata.query_backend.empty() ? "unavailable" : metadata.query_backend)
              << "\n";
    std::cout << "Query backend available: "
              << yesNo(model->queryBackendAvailable()) << "\n";
    printAABB(model->boundingBox());
    std::cout << "Fine cell count: " << metadata.n_fine_cell << "\n";
    std::cout << "Fine node count: " << metadata.n_fine_node << "\n";
    std::cout << "Fine spacing: " << metadata.h_fine << "\n";
    std::cout << "Max level: " << metadata.max_level << "\n";
    std::cout << "Final blocks: " << metadata.final_block_count << "\n";
    std::cout << "Active blocks: " << metadata.active_block_count << "\n";
    std::cout << "Near-surface blocks: " << metadata.near_surface_block_count << "\n";
    std::cout << "Near-surface error: " << metadata.near_surface_error << "\n";
    std::cout << "Max reconstruction error: "
              << metadata.max_reconstruction_error << "\n";
    std::cout << "Compression ratio: " << metadata.compression_ratio << "\n";
    std::cout << "Memory footprint bytes: " << model->memoryFootprintBytes() << "\n";
    std::cout << "Memory footprint MB: " << std::fixed << std::setprecision(3)
              << memory_mb << "\n";
    return model->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_info failed: " << exc.what() << "\n";
    return 1;
  }
}
