#include "adasdf/io/CompressedBlockSDFBin.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace adasdf {
namespace {

std::string trimCarriageReturn(std::string value) {
  if (!value.empty() && value.back() == '\r') {
    value.pop_back();
  }
  return value;
}

void requireFinite(const Vector3& value, const char* label) {
  if (!value.allFinite()) {
    throw std::runtime_error(
        std::string("Compressed block .sdfbin ") + label + " must be finite.");
  }
}

void requirePositive(const Vector3& value, const char* label) {
  requireFinite(value, label);
  if (!(value.x > 0.0) || !(value.y > 0.0) || !(value.z > 0.0)) {
    throw std::runtime_error(
        std::string("Compressed block .sdfbin ") + label + " must be positive.");
  }
}

std::size_t valueCount(const CompressedSDFBlock& block) {
  return static_cast<std::size_t>(block.nx) *
         static_cast<std::size_t>(block.ny) *
         static_cast<std::size_t>(block.nz);
}

template <typename T>
void readVector(std::istream& input, std::vector<T>* values, std::size_t count) {
  values->clear();
  values->reserve(count);
  T value{};
  for (std::size_t i = 0; i < count; ++i) {
    if (!(input >> value)) {
      throw std::runtime_error("Compressed block .sdfbin ended while reading values.");
    }
    values->push_back(value);
  }
}

void writeValues(std::ostream& output, const std::vector<double>& values) {
  for (const double value : values) {
    output << value << "\n";
  }
}

}  // namespace

const char* CompressedBlockSDFBin::magic() {
  return "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1";
}

bool CompressedBlockSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string first_line;
  std::getline(input, first_line);
  return trimCarriageReturn(first_line) == magic();
}

std::shared_ptr<CompressedAdaptiveBlockSDFModel> CompressedBlockSDFBin::read(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Could not open compressed block .sdfbin: " + path.string());
  }

  std::string line;
  std::getline(input, line);
  if (trimCarriageReturn(line) != magic()) {
    throw std::runtime_error("File is not an ADASDF compressed block .sdfbin.");
  }

  CompressedAdaptiveBlockSDF compressed;
  int expected_blocks = -1;
  bool saw_global_min = false;
  bool saw_global_max = false;

  while (std::getline(input, line)) {
    line = trimCarriageReturn(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream tokens(line);
    std::string key;
    tokens >> key;
    if (key == "unit") {
      std::string unit;
      tokens >> unit;
    } else if (key == "signed") {
      int signed_flag = 1;
      tokens >> signed_flag;
      compressed.signed_distance = signed_flag != 0;
    } else if (key == "global_min") {
      tokens >> compressed.global_bounds.min.x >> compressed.global_bounds.min.y >>
          compressed.global_bounds.min.z;
      saw_global_min = true;
    } else if (key == "global_max") {
      tokens >> compressed.global_bounds.max.x >> compressed.global_bounds.max.y >>
          compressed.global_bounds.max.z;
      saw_global_max = true;
    } else if (key == "block_count") {
      tokens >> expected_blocks;
    } else if (key == "compression_method" ||
               key == "original_memory_bytes" ||
               key == "compressed_memory_bytes" ||
               key == "compression_ratio") {
      continue;
    } else if (key == "block") {
      CompressedSDFBlock block;
      block.signed_distance = compressed.signed_distance;
      bool saw_payload = false;
      while (std::getline(input, line)) {
        line = trimCarriageReturn(line);
        if (line.empty() || line[0] == '#') {
          continue;
        }
        std::istringstream block_tokens(line);
        std::string block_key;
        block_tokens >> block_key;
        if (block_key == "id") {
          block_tokens >> block.block_id;
        } else if (block_key == "source_block_id") {
          block_tokens >> block.source_block_id;
        } else if (block_key == "node_id") {
          block_tokens >> block.octree_node_id;
        } else if (block_key == "level") {
          block_tokens >> block.level;
        } else if (block_key == "near_surface") {
          int flag = 0;
          block_tokens >> flag;
          block.near_surface = flag != 0;
        } else if (block_key == "method") {
          std::string method;
          block_tokens >> method;
          block.method = blockCompressionMethodFromName(method);
        } else if (block_key == "min") {
          block_tokens >> block.bounds.min.x >> block.bounds.min.y >>
              block.bounds.min.z;
          block.bounds.valid = true;
        } else if (block_key == "max") {
          block_tokens >> block.bounds.max.x >> block.bounds.max.y >>
              block.bounds.max.z;
          block.bounds.valid = true;
        } else if (block_key == "origin") {
          block_tokens >> block.origin.x >> block.origin.y >> block.origin.z;
        } else if (block_key == "spacing") {
          block_tokens >> block.spacing.x >> block.spacing.y >> block.spacing.z;
        } else if (block_key == "resolution") {
          block_tokens >> block.nx >> block.ny >> block.nz;
        } else if (block_key == "rank") {
          block_tokens >> block.svd.rank;
        } else if (block_key == "error") {
          block_tokens >> block.max_abs_error >> block.mean_abs_error >>
              block.rms_error >> block.p95_abs_error;
        } else if (block_key == "original_bytes") {
          block_tokens >> block.original_bytes;
        } else if (block_key == "compressed_bytes") {
          block_tokens >> block.compressed_bytes;
        } else if (block_key == "U") {
          block.svd.rows = block.nx;
          block.svd.cols = block.ny * block.nz;
          block.svd.nx = block.nx;
          block.svd.ny = block.ny;
          block.svd.nz = block.nz;
          readVector(
              input,
              &block.svd.U,
              static_cast<std::size_t>(block.svd.rows) *
                  static_cast<std::size_t>(block.svd.rank));
        } else if (block_key == "S") {
          readVector(input, &block.svd.S, static_cast<std::size_t>(block.svd.rank));
        } else if (block_key == "Vt") {
          readVector(
              input,
              &block.svd.Vt,
              static_cast<std::size_t>(block.svd.rank) *
                  static_cast<std::size_t>(block.svd.cols));
        } else if (block_key == "values") {
          readVector(input, &block.dense_phi, valueCount(block));
        } else if (block_key == "end_block") {
          saw_payload = true;
          break;
        } else {
          throw std::runtime_error(
              "Unknown key in compressed block .sdfbin block: " + block_key);
        }
        if (!block_tokens && block_key != "U" && block_key != "S" &&
            block_key != "Vt" && block_key != "values") {
          throw std::runtime_error(
              "Malformed line in compressed block .sdfbin: " + line);
        }
      }
      if (!saw_payload) {
        throw std::runtime_error("Compressed block .sdfbin block missing end_block.");
      }
      requireFinite(block.bounds.min, "block min");
      requireFinite(block.bounds.max, "block max");
      requireFinite(block.origin, "block origin");
      requirePositive(block.spacing, "block spacing");
      block.compression_success = true;
      compressed.blocks.push_back(std::move(block));
    } else {
      throw std::runtime_error("Unknown key in compressed block .sdfbin: " + key);
    }
    if (!tokens) {
      throw std::runtime_error("Malformed line in compressed block .sdfbin: " + line);
    }
  }

  compressed.global_bounds.valid = saw_global_min && saw_global_max;
  requireFinite(compressed.global_bounds.min, "global_min");
  requireFinite(compressed.global_bounds.max, "global_max");
  if (expected_blocks < 0 ||
      compressed.blocks.size() != static_cast<std::size_t>(expected_blocks)) {
    throw std::runtime_error(
        "Compressed block .sdfbin block count does not match header.");
  }

  auto model =
      std::make_shared<CompressedAdaptiveBlockSDFModel>(std::move(compressed));
  if (!model->isValid()) {
    throw std::runtime_error(
        "Compressed block .sdfbin produced an invalid model.");
  }
  SDFMetadata metadata = model->metadata();
  metadata.source_path = path.string();
  model->setMetadata(metadata);
  return model;
}

void CompressedBlockSDFBin::write(
    const std::filesystem::path& path,
    const CompressedAdaptiveBlockSDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("CompressedBlockSDFBin::write received an empty path.");
  }
  if (!model.isValid() ||
      !CompressedAdaptiveBlockSDFModel::isValidCompressedBlockSet(
          model.compressedBlockSet())) {
    throw std::runtime_error(
        "CompressedBlockSDFBin::write requires a valid compressed model.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    throw std::runtime_error(
        "Could not write compressed block .sdfbin: " + path.string());
  }

  const CompressedAdaptiveBlockSDF& compressed = model.compressedBlockSet();
  output << std::setprecision(std::numeric_limits<double>::max_digits10);
  output << magic() << "\n";
  output << "unit m\n";
  output << "signed " << (compressed.signed_distance ? 1 : 0) << "\n";
  output << "global_min " << compressed.global_bounds.min << "\n";
  output << "global_max " << compressed.global_bounds.max << "\n";
  output << "block_count " << compressed.blocks.size() << "\n";
  output << "compression_method mixed_matrix_svd_dense_fallback\n";
  output << "original_memory_bytes " << compressed.originalMemoryBytes() << "\n";
  output << "compressed_memory_bytes " << compressed.compressedMemoryBytes() << "\n";
  output << "compression_ratio " << compressed.compressionRatio() << "\n";

  for (const CompressedSDFBlock& block : compressed.blocks) {
    output << "block\n";
    output << "id " << block.block_id << "\n";
    output << "source_block_id " << block.source_block_id << "\n";
    output << "node_id " << block.octree_node_id << "\n";
    output << "level " << block.level << "\n";
    output << "near_surface " << (block.near_surface ? 1 : 0) << "\n";
    output << "method " << blockCompressionMethodName(block.method) << "\n";
    output << "min " << block.bounds.min << "\n";
    output << "max " << block.bounds.max << "\n";
    output << "origin " << block.origin << "\n";
    output << "spacing " << block.spacing << "\n";
    output << "resolution " << block.nx << " " << block.ny << " "
           << block.nz << "\n";
    output << "error " << block.max_abs_error << " " << block.mean_abs_error
           << " " << block.rms_error << " " << block.p95_abs_error << "\n";
    output << "original_bytes " << block.original_bytes << "\n";
    output << "compressed_bytes " << block.compressed_bytes << "\n";
    if (block.method == BlockCompressionMethod::MatrixSVD) {
      output << "rank " << block.svd.rank << "\n";
      output << "U\n";
      writeValues(output, block.svd.U);
      output << "S\n";
      writeValues(output, block.svd.S);
      output << "Vt\n";
      writeValues(output, block.svd.Vt);
    } else {
      output << "values\n";
      writeValues(output, block.dense_phi);
    }
    output << "end_block\n";
  }
}

}  // namespace adasdf
