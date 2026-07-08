#pragma once

#include <adasdf/adasdf.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>

namespace adasdf_tools {

inline std::string modelType(const adasdf::SDFModel& model) {
  if (dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(&model)) {
    return "CompressedAdaptiveBlockSDF";
  }
  if (dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model)) {
    return "AdaptiveBlockSDF";
  }
  if (dynamic_cast<const adasdf::DenseSDFModel*>(&model)) {
    return "DenseSDF";
  }
  if (dynamic_cast<const adasdf::DemoAdaptiveSDFModel*>(&model)) {
    return "DemoAdaptiveSDF";
  }
  if (dynamic_cast<const adasdf::AnalyticSDFModel*>(&model)) {
    return "AnalyticSDF";
  }
  return model.metadata().model_name.empty() ? model.debugName()
                                             : model.metadata().model_name;
}

inline std::string resolutionJson(int nx, int ny, int nz) {
  return "{\"x\":" + adasdf::JsonContractWriter::integerSigned(nx) +
         ",\"y\":" + adasdf::JsonContractWriter::integerSigned(ny) +
         ",\"z\":" + adasdf::JsonContractWriter::integerSigned(nz) + "}";
}

inline std::string denseGridJson(const adasdf::SDFModel& model) {
  const auto* dense = dynamic_cast<const adasdf::DenseSDFModel*>(&model);
  if (!dense) {
    return "null";
  }
  const adasdf::DenseSDFGrid& grid = dense->grid();
  return "{\"resolution\":" + resolutionJson(grid.nx, grid.ny, grid.nz) +
         ",\"origin\":" + adasdf::JsonContractWriter::vec3(grid.origin) +
         ",\"spacing\":" + adasdf::JsonContractWriter::vec3(grid.spacing) +
         ",\"signed\":" +
         adasdf::JsonContractWriter::boolean(grid.signed_distance) +
         ",\"unit\":" + adasdf::JsonContractWriter::quote(grid.unit) + "}";
}

inline std::string capabilitiesJson(const adasdf::SDFModel& model) {
  return "{\"query_backend_available\":" +
         adasdf::JsonContractWriter::boolean(model.queryBackendAvailable()) +
         ",\"dense_grid\":" +
         adasdf::JsonContractWriter::boolean(
             dynamic_cast<const adasdf::DenseSDFModel*>(&model) != nullptr) +
         ",\"adaptive_blocks\":" +
         adasdf::JsonContractWriter::boolean(
             dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model) !=
                 nullptr) +
         ",\"compressed_blocks\":" +
         adasdf::JsonContractWriter::boolean(
             dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(
                 &model) != nullptr) +
         ",\"gpu_native_compressed_query\":false}";
}

inline std::string metadataBlocksJson(const adasdf::SDFModel& model) {
  std::ostringstream out;
  out << "[";
  const auto& metadata = model.blockMetadata();
  for (std::size_t i = 0; i < metadata.size(); ++i) {
    const auto& block = metadata[i];
    if (i != 0) {
      out << ",";
    }
    adasdf::AABB aabb;
    aabb.min = block.local_min;
    aabb.max = block.local_max;
    aabb.valid = true;
    const std::size_t sample_count =
        static_cast<std::size_t>(std::max(0, block.resolution.x)) *
        static_cast<std::size_t>(std::max(0, block.resolution.y)) *
        static_cast<std::size_t>(std::max(0, block.resolution.z));
    out << "{\"block_id\":" << block.block_id
        << ",\"level\":0,\"aabb\":" << adasdf::JsonContractWriter::aabb(aabb)
        << ",\"resolution\":"
        << resolutionJson(
               block.resolution.x,
               block.resolution.y,
               block.resolution.z)
        << ",\"sample_count\":" << sample_count
        << ",\"near_surface\":false,\"contact_band\":false"
        << ",\"compression_rank\":" << block.compression_rank << "}";
  }
  out << "]";
  return out.str();
}

inline std::string adaptiveBlocksJson(
    const adasdf::AdaptiveBlockSDFModel& model) {
  std::ostringstream out;
  out << "[";
  const auto& blocks = model.blockSet().blocks;
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    const auto& block = blocks[i];
    if (i != 0) {
      out << ",";
    }
    const std::size_t sample_count =
        static_cast<std::size_t>(std::max(0, block.nx)) *
        static_cast<std::size_t>(std::max(0, block.ny)) *
        static_cast<std::size_t>(std::max(0, block.nz));
    out << "{\"block_id\":" << block.block_id
        << ",\"octree_node_id\":" << block.octree_node_id
        << ",\"level\":" << block.level
        << ",\"aabb\":" << adasdf::JsonContractWriter::aabb(block.bounds)
        << ",\"resolution\":" << resolutionJson(block.nx, block.ny, block.nz)
        << ",\"sample_count\":" << sample_count
        << ",\"near_surface\":"
        << adasdf::JsonContractWriter::boolean(block.near_surface)
        << ",\"contact_band\":false,\"active_cache\":null}";
  }
  out << "]";
  return out.str();
}

inline std::string compressedBlocksJson(
    const adasdf::CompressedAdaptiveBlockSDFModel& model) {
  std::ostringstream out;
  out << "[";
  const auto& blocks = model.compressedBlockSet().blocks;
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    const auto& block = blocks[i];
    if (i != 0) {
      out << ",";
    }
    const std::size_t sample_count =
        static_cast<std::size_t>(std::max(0, block.nx)) *
        static_cast<std::size_t>(std::max(0, block.ny)) *
        static_cast<std::size_t>(std::max(0, block.nz));
    out << "{\"block_id\":" << block.block_id
        << ",\"source_block_id\":" << block.source_block_id
        << ",\"octree_node_id\":" << block.octree_node_id
        << ",\"level\":" << block.level
        << ",\"aabb\":" << adasdf::JsonContractWriter::aabb(block.bounds)
        << ",\"resolution\":" << resolutionJson(block.nx, block.ny, block.nz)
        << ",\"sample_count\":" << sample_count
        << ",\"near_surface\":"
        << adasdf::JsonContractWriter::boolean(block.near_surface)
        << ",\"contact_band\":false,\"active_cache\":null"
        << ",\"compression_method\":"
        << adasdf::JsonContractWriter::quote(
               adasdf::blockCompressionMethodName(block.method))
        << ",\"rank\":" << block.svd.rank
        << ",\"max_abs_error\":"
        << adasdf::JsonContractWriter::number(block.max_abs_error) << "}";
  }
  out << "]";
  return out.str();
}

inline std::string blocksJson(const adasdf::SDFModel& model) {
  if (const auto* compressed =
          dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(
              &model)) {
    return compressedBlocksJson(*compressed);
  }
  if (const auto* adaptive =
          dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model)) {
    return adaptiveBlocksJson(*adaptive);
  }
  return metadataBlocksJson(model);
}

inline std::string levelsJson(const adasdf::SDFModel& model) {
  std::set<int> levels;
  if (const auto* compressed =
          dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(
              &model)) {
    for (const auto& block : compressed->compressedBlockSet().blocks) {
      levels.insert(block.level);
    }
  } else if (const auto* adaptive =
                 dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model)) {
    for (const auto& block : adaptive->blockSet().blocks) {
      levels.insert(block.level);
    }
  } else if (model.metadata().max_level > 0) {
    levels.insert(model.metadata().max_level);
  }
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (int level : levels) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << level;
  }
  out << "]";
  return out.str();
}

inline std::size_t blockCount(const adasdf::SDFModel& model) {
  if (const auto* compressed =
          dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(
              &model)) {
    return compressed->compressedBlockSet().blocks.size();
  }
  if (const auto* adaptive =
          dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model)) {
    return adaptive->blockSet().blocks.size();
  }
  return model.blockMetadata().size();
}

inline std::string compressionSummaryJson(const adasdf::SDFModel& model) {
  const auto* compressed =
      dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(&model);
  if (!compressed) {
    return "{\"compressed\":false}";
  }
  const auto& blocks = compressed->compressedBlockSet();
  std::size_t matrix_svd_count = 0;
  std::size_t dense_fallback_count = 0;
  std::size_t near_surface_count = 0;
  int min_rank = 0;
  int max_rank = 0;
  double rank_sum = 0.0;
  double max_error = 0.0;
  double mean_error_sum = 0.0;
  for (const auto& block : blocks.blocks) {
    if (block.method == adasdf::BlockCompressionMethod::MatrixSVD) {
      ++matrix_svd_count;
      min_rank = min_rank == 0 ? block.svd.rank : std::min(min_rank, block.svd.rank);
      max_rank = std::max(max_rank, block.svd.rank);
      rank_sum += static_cast<double>(block.svd.rank);
    } else if (block.method == adasdf::BlockCompressionMethod::DenseFallback) {
      ++dense_fallback_count;
    }
    if (block.near_surface) {
      ++near_surface_count;
    }
    max_error = std::max(max_error, block.max_abs_error);
    mean_error_sum += block.mean_abs_error;
  }
  const double avg_rank = matrix_svd_count > 0
      ? rank_sum / static_cast<double>(matrix_svd_count)
      : 0.0;
  const double mean_error = !blocks.blocks.empty()
      ? mean_error_sum / static_cast<double>(blocks.blocks.size())
      : 0.0;
  return "{\"compressed\":true,\"block_counts\":{\"total\":" +
         adasdf::JsonContractWriter::integer(blocks.blocks.size()) +
         ",\"matrix_svd\":" +
         adasdf::JsonContractWriter::integer(matrix_svd_count) +
         ",\"dense_fallback\":" +
         adasdf::JsonContractWriter::integer(dense_fallback_count) +
         ",\"near_surface\":" +
         adasdf::JsonContractWriter::integer(near_surface_count) +
         "},\"rank_stats\":{\"min\":" + std::to_string(min_rank) +
         ",\"max\":" + std::to_string(max_rank) +
         ",\"mean\":" + adasdf::JsonContractWriter::number(avg_rank) +
         "},\"memory\":{\"original_bytes\":" +
         adasdf::JsonContractWriter::integer(blocks.originalMemoryBytes()) +
         ",\"compressed_bytes\":" +
         adasdf::JsonContractWriter::integer(blocks.compressedMemoryBytes()) +
         "},\"compression_ratio\":" +
         adasdf::JsonContractWriter::number(blocks.compressionRatio()) +
         ",\"error_summary\":{\"max_abs_error\":" +
         adasdf::JsonContractWriter::number(max_error) +
         ",\"mean_abs_error\":" +
         adasdf::JsonContractWriter::number(mean_error) + "}}";
}

inline std::string compressionPayloadField(
    const adasdf::SDFModel& model,
    const std::string& field) {
  const std::string summary = compressionSummaryJson(model);
  const auto* compressed =
      dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(&model);
  if (!compressed) {
    if (field == "block_counts") {
      return "{\"total\":0,\"matrix_svd\":0,\"dense_fallback\":0,\"near_surface\":0}";
    }
    if (field == "rank_stats") {
      return "{\"min\":0,\"max\":0,\"mean\":0}";
    }
    if (field == "memory") {
      return "{\"original_bytes\":0,\"compressed_bytes\":0}";
    }
    if (field == "compression_ratio") {
      return "1";
    }
    if (field == "error_summary") {
      return "{\"max_abs_error\":0,\"mean_abs_error\":0}";
    }
  }
  const auto& blocks = compressed->compressedBlockSet();
  std::size_t matrix_svd_count = 0;
  std::size_t dense_fallback_count = 0;
  std::size_t near_surface_count = 0;
  int min_rank = 0;
  int max_rank = 0;
  double rank_sum = 0.0;
  double max_error = 0.0;
  double mean_error_sum = 0.0;
  for (const auto& block : blocks.blocks) {
    if (block.method == adasdf::BlockCompressionMethod::MatrixSVD) {
      ++matrix_svd_count;
      min_rank = min_rank == 0 ? block.svd.rank : std::min(min_rank, block.svd.rank);
      max_rank = std::max(max_rank, block.svd.rank);
      rank_sum += static_cast<double>(block.svd.rank);
    } else if (block.method == adasdf::BlockCompressionMethod::DenseFallback) {
      ++dense_fallback_count;
    }
    if (block.near_surface) {
      ++near_surface_count;
    }
    max_error = std::max(max_error, block.max_abs_error);
    mean_error_sum += block.mean_abs_error;
  }
  const double avg_rank = matrix_svd_count > 0
      ? rank_sum / static_cast<double>(matrix_svd_count)
      : 0.0;
  const double mean_error = !blocks.blocks.empty()
      ? mean_error_sum / static_cast<double>(blocks.blocks.size())
      : 0.0;
  if (field == "block_counts") {
    return "{\"total\":" +
           adasdf::JsonContractWriter::integer(blocks.blocks.size()) +
           ",\"matrix_svd\":" +
           adasdf::JsonContractWriter::integer(matrix_svd_count) +
           ",\"dense_fallback\":" +
           adasdf::JsonContractWriter::integer(dense_fallback_count) +
           ",\"near_surface\":" +
           adasdf::JsonContractWriter::integer(near_surface_count) + "}";
  }
  if (field == "rank_stats") {
    return "{\"min\":" + std::to_string(min_rank) +
           ",\"max\":" + std::to_string(max_rank) +
           ",\"mean\":" + adasdf::JsonContractWriter::number(avg_rank) + "}";
  }
  if (field == "memory") {
    return "{\"original_bytes\":" +
           adasdf::JsonContractWriter::integer(blocks.originalMemoryBytes()) +
           ",\"compressed_bytes\":" +
           adasdf::JsonContractWriter::integer(blocks.compressedMemoryBytes()) +
           "}";
  }
  if (field == "compression_ratio") {
    return adasdf::JsonContractWriter::number(blocks.compressionRatio());
  }
  if (field == "error_summary") {
    return "{\"max_abs_error\":" +
           adasdf::JsonContractWriter::number(max_error) +
           ",\"mean_abs_error\":" +
           adasdf::JsonContractWriter::number(mean_error) + "}";
  }
  return summary;
}

inline adasdf::BackendJsonContract makeBaseContract(
    const char* schema_id,
    const char* tool_name) {
  adasdf::BackendJsonContract contract;
  contract.schema_id = schema_id;
  contract.tool_name = tool_name;
  contract.status = adasdf::JsonStatus::Ok;
  contract.status_code = adasdf::ErrorCode::OK;
  contract.success = true;
  return contract;
}

}  // namespace adasdf_tools
