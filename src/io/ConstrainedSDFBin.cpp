#include "adasdf/io/ConstrainedSDFBin.h"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <type_traits>

namespace adasdf {
namespace {

constexpr std::size_t kMagicBytes = 16;
constexpr std::uint32_t kEndianMarker = 0x01020304u;
constexpr std::size_t kBlockRecordBytes =
    3 * sizeof(std::int32_t) + 6 * sizeof(double) +
    8 * sizeof(std::int32_t) + 2 * sizeof(double) +
    6 * sizeof(std::uint64_t);

std::uint64_t checksum(const unsigned char* data, std::size_t size) {
  std::uint64_t value = 1469598103934665603ull;
  for (std::size_t i = 0; i < size; ++i) {
    value ^= static_cast<std::uint64_t>(data[i]);
    value *= 1099511628211ull;
  }
  return value;
}

template <typename T>
void append(std::vector<unsigned char>* bytes, const T& value) {
  static_assert(std::is_trivially_copyable<T>::value, "binary POD required");
  const unsigned char* begin =
      reinterpret_cast<const unsigned char*>(&value);
  bytes->insert(bytes->end(), begin, begin + sizeof(T));
}

void appendRaw(
    std::vector<unsigned char>* bytes,
    const void* data,
    std::size_t size) {
  const unsigned char* begin = reinterpret_cast<const unsigned char*>(data);
  bytes->insert(bytes->end(), begin, begin + size);
}

template <typename T>
bool read(
    const std::vector<unsigned char>& bytes,
    std::size_t* offset,
    T* value) {
  static_assert(std::is_trivially_copyable<T>::value, "binary POD required");
  if (value == nullptr || offset == nullptr ||
      *offset > bytes.size() || bytes.size() - *offset < sizeof(T)) {
    return false;
  }
  std::memcpy(value, bytes.data() + *offset, sizeof(T));
  *offset += sizeof(T);
  return true;
}

void appendVector(
    std::vector<unsigned char>* bytes,
    const std::vector<double>& values) {
  append(bytes, static_cast<std::uint64_t>(values.size()));
  if (!values.empty()) {
    appendRaw(bytes, values.data(), values.size() * sizeof(double));
  }
}

bool readVector(
    const std::vector<unsigned char>& bytes,
    std::size_t* offset,
    std::vector<double>* values) {
  std::uint64_t count = 0;
  if (!read(bytes, offset, &count) ||
      count > static_cast<std::uint64_t>(
          std::numeric_limits<std::size_t>::max() / sizeof(double)) ||
      *offset > bytes.size() ||
      bytes.size() - *offset < static_cast<std::size_t>(count) * sizeof(double)) {
    return false;
  }
  values->resize(static_cast<std::size_t>(count));
  if (count > 0) {
    std::memcpy(
        values->data(), bytes.data() + *offset,
        static_cast<std::size_t>(count) * sizeof(double));
    *offset += static_cast<std::size_t>(count) * sizeof(double);
  }
  return true;
}

std::vector<unsigned char> tensorPayload(const CompressedTensor3D& tensor) {
  std::vector<unsigned char> out;
  appendVector(&out, tensor.dense);
  appendVector(&out, tensor.matrix_u);
  appendVector(&out, tensor.matrix_s);
  appendVector(&out, tensor.matrix_vt);
  appendVector(&out, tensor.factor_x);
  appendVector(&out, tensor.factor_y);
  appendVector(&out, tensor.factor_z);
  appendVector(&out, tensor.core);
  appendVector(&out, tensor.tt_core_1);
  appendVector(&out, tensor.tt_core_2);
  appendVector(&out, tensor.tt_core_3);
  return out;
}

void appendAABB(std::vector<unsigned char>* bytes, const AABB& bounds) {
  append(bytes, bounds.min.x);
  append(bytes, bounds.min.y);
  append(bytes, bounds.min.z);
  append(bytes, bounds.max.x);
  append(bytes, bounds.max.y);
  append(bytes, bounds.max.z);
}

bool readAABB(
    const std::vector<unsigned char>& bytes,
    std::size_t* offset,
    AABB* bounds) {
  bounds->valid = read(bytes, offset, &bounds->min.x) &&
      read(bytes, offset, &bounds->min.y) &&
      read(bytes, offset, &bounds->min.z) &&
      read(bytes, offset, &bounds->max.x) &&
      read(bytes, offset, &bounds->max.y) &&
      read(bytes, offset, &bounds->max.z);
  return bounds->valid;
}

std::vector<unsigned char> readFile(
    const std::filesystem::path& path,
    std::string* error) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    if (error != nullptr) {
      *error = "failed to open constrained SDF file";
    }
    return {};
  }
  const std::streamoff length = input.tellg();
  if (length <= 0 ||
      static_cast<std::uint64_t>(length) >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    if (error != nullptr) {
      *error = "invalid constrained SDF file length";
    }
    return {};
  }
  std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
  input.seekg(0);
  input.read(reinterpret_cast<char*>(bytes.data()), length);
  if (!input) {
    if (error != nullptr) {
      *error = "failed to read constrained SDF file";
    }
    return {};
  }
  return bytes;
}

void appendOctreeNode(
    std::vector<unsigned char>* bytes,
    const ConstrainedAdaptiveOctreeNode& node) {
  append(bytes, static_cast<std::int32_t>(node.node_id));
  append(bytes, static_cast<std::int32_t>(node.parent_id));
  append(bytes, static_cast<std::int32_t>(node.level));
  append(bytes, static_cast<std::uint8_t>(node.is_leaf ? 1 : 0));
  append(bytes, static_cast<std::uint8_t>(node.near_surface ? 1 : 0));
  append(bytes, static_cast<std::uint8_t>(node.sign_change ? 1 : 0));
  append(bytes, static_cast<std::uint8_t>(0));
  appendAABB(bytes, node.bounds);
  for (const int child : node.child_ids) {
    append(bytes, static_cast<std::int32_t>(child));
  }
}

bool readOctreeNode(
    const std::vector<unsigned char>& bytes,
    std::size_t* offset,
    ConstrainedAdaptiveOctreeNode* node) {
  std::int32_t id = -1;
  std::int32_t parent = -1;
  std::int32_t level = 0;
  std::uint8_t leaf = 0;
  std::uint8_t near_surface = 0;
  std::uint8_t sign_change = 0;
  std::uint8_t reserved = 0;
  if (!read(bytes, offset, &id) || !read(bytes, offset, &parent) ||
      !read(bytes, offset, &level) || !read(bytes, offset, &leaf) ||
      !read(bytes, offset, &near_surface) ||
      !read(bytes, offset, &sign_change) ||
      !read(bytes, offset, &reserved) || !readAABB(bytes, offset, &node->bounds)) {
    return false;
  }
  node->node_id = id;
  node->parent_id = parent;
  node->level = level;
  node->is_leaf = leaf != 0;
  node->near_surface = near_surface != 0;
  node->sign_change = sign_change != 0;
  for (int& child : node->child_ids) {
    std::int32_t value = -1;
    if (!read(bytes, offset, &value)) {
      return false;
    }
    child = value;
  }
  return true;
}

void appendBlockRecord(
    std::vector<unsigned char>* bytes,
    const ConstrainedSDFBinBlockRecord& block) {
  append(bytes, static_cast<std::int32_t>(block.block_id));
  append(bytes, static_cast<std::int32_t>(block.source_octree_node_id));
  append(bytes, static_cast<std::int32_t>(block.level));
  appendAABB(bytes, block.bounds);
  append(bytes, static_cast<std::int32_t>(block.nx));
  append(bytes, static_cast<std::int32_t>(block.ny));
  append(bytes, static_cast<std::int32_t>(block.nz));
  append(bytes, static_cast<std::int32_t>(block.method));
  append(bytes, block.tolerance);
  append(bytes, block.actual_max_abs_error);
  append(bytes, static_cast<std::int32_t>(block.rank_x));
  append(bytes, static_cast<std::int32_t>(block.rank_y));
  append(bytes, static_cast<std::int32_t>(block.rank_z));
  append(bytes, static_cast<std::int32_t>(0));
  append(bytes, block.original_bytes);
  append(bytes, block.compressed_bytes);
  append(bytes, block.decoded_peak_bytes);
  append(bytes, block.payload_offset);
  append(bytes, block.payload_bytes);
  append(bytes, block.payload_checksum);
}

bool readBlockRecord(
    const std::vector<unsigned char>& bytes,
    std::size_t* offset,
    ConstrainedSDFBinBlockRecord* block) {
  const std::size_t begin = *offset;
  std::int32_t id = -1;
  std::int32_t source = -1;
  std::int32_t level = 0;
  std::int32_t nx = 0;
  std::int32_t ny = 0;
  std::int32_t nz = 0;
  std::int32_t method = 0;
  std::int32_t rx = 0;
  std::int32_t ry = 0;
  std::int32_t rz = 0;
  std::int32_t reserved = 0;
  if (!read(bytes, offset, &id) || !read(bytes, offset, &source) ||
      !read(bytes, offset, &level) || !readAABB(bytes, offset, &block->bounds) ||
      !read(bytes, offset, &nx) || !read(bytes, offset, &ny) ||
      !read(bytes, offset, &nz) || !read(bytes, offset, &method) ||
      !read(bytes, offset, &block->tolerance) ||
      !read(bytes, offset, &block->actual_max_abs_error) ||
      !read(bytes, offset, &rx) || !read(bytes, offset, &ry) ||
      !read(bytes, offset, &rz) || !read(bytes, offset, &reserved) ||
      !read(bytes, offset, &block->original_bytes) ||
      !read(bytes, offset, &block->compressed_bytes) ||
      !read(bytes, offset, &block->decoded_peak_bytes) ||
      !read(bytes, offset, &block->payload_offset) ||
      !read(bytes, offset, &block->payload_bytes) ||
      !read(bytes, offset, &block->payload_checksum) ||
      *offset - begin != kBlockRecordBytes) {
    return false;
  }
  if (method < static_cast<int>(TensorCompressionMethod::Dense) ||
      method > static_cast<int>(TensorCompressionMethod::Tucker)) {
    return false;
  }
  block->block_id = id;
  block->source_octree_node_id = source;
  block->level = level;
  block->nx = nx;
  block->ny = ny;
  block->nz = nz;
  block->method = static_cast<TensorCompressionMethod>(method);
  block->rank_x = rx;
  block->rank_y = ry;
  block->rank_z = rz;
  return nx >= 2 && ny >= 2 && nz >= 2;
}

}  // namespace

const char* ConstrainedSDFBin::magic() {
  return "ADASDF_C3D_V1";
}

bool ConstrainedSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return false;
  }
  std::array<char, kMagicBytes> actual{};
  input.read(actual.data(), static_cast<std::streamsize>(actual.size()));
  std::array<char, kMagicBytes> expected{};
  std::memcpy(
      expected.data(), magic(),
      std::min<std::size_t>(std::strlen(magic()), expected.size()));
  return input.gcount() == static_cast<std::streamsize>(actual.size()) &&
      actual == expected;
}

bool ConstrainedSDFBin::write(
    const std::filesystem::path& path,
    const ConstrainedSDFAsset& asset,
    ConstrainedSDFBinReport* report_out) {
  ConstrainedSDFBinReport report;
  report.format_version = kConstrainedSDFBinFormatVersion;
  if (path.empty() || asset.octree.nodes.empty() || asset.blocks.empty()) {
    report.error_message = "invalid constrained SDF asset or output path";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return false;
  }
  std::vector<std::vector<unsigned char>> payloads;
  std::vector<ConstrainedSDFBinBlockRecord> records;
  payloads.reserve(asset.blocks.size());
  records.reserve(asset.blocks.size());
  for (const ConstrainedSDFAssetBlock& source : asset.blocks) {
    std::vector<unsigned char> payload = tensorPayload(source.tensor);
    if (payload.empty()) {
      report.error_message = "empty constrained SDF block payload";
      if (report_out != nullptr) {
        *report_out = report;
      }
      return false;
    }
    ConstrainedSDFBinBlockRecord record;
    record.block_id = source.plan.block_id;
    record.source_octree_node_id = source.plan.source_octree_node_id;
    record.level = source.plan.level;
    record.bounds = source.plan.bounds;
    record.nx = source.tensor.nx;
    record.ny = source.tensor.ny;
    record.nz = source.tensor.nz;
    record.method = source.tensor.method;
    record.rank_x = source.tensor.rank_x;
    record.rank_y = source.tensor.rank_y;
    record.rank_z = source.tensor.rank_z;
    record.tolerance = source.tensor.tolerance;
    record.actual_max_abs_error = source.tensor.actual_max_abs_error;
    record.original_bytes = source.tensor.original_bytes;
    record.compressed_bytes = source.tensor.compressed_bytes;
    record.decoded_peak_bytes = source.tensor.decoded_peak_bytes;
    record.payload_bytes = payload.size();
    record.payload_checksum = checksum(payload.data(), payload.size());
    payloads.push_back(std::move(payload));
    records.push_back(record);
  }

  std::vector<unsigned char> bytes;
  std::array<char, kMagicBytes> magic_bytes{};
  std::memcpy(
      magic_bytes.data(), magic(),
      std::min<std::size_t>(std::strlen(magic()), magic_bytes.size()));
  appendRaw(&bytes, magic_bytes.data(), magic_bytes.size());
  append(&bytes, kConstrainedSDFBinFormatVersion);
  append(&bytes, kEndianMarker);
  const std::size_t file_size_offset = bytes.size();
  append(&bytes, std::uint64_t{0});
  append(&bytes, static_cast<std::uint64_t>(asset.octree.nodes.size()));
  append(&bytes, static_cast<std::uint64_t>(asset.blocks.size()));
  appendAABB(&bytes, asset.octree.bounds);
  append(&bytes, asset.max_zero_surface_abs_error);
  append(&bytes, static_cast<std::uint8_t>(
      asset.zero_surface_error_is_strict_bound ? 1 : 0));
  for (int i = 0; i < 7; ++i) {
    append(&bytes, static_cast<std::uint8_t>(0));
  }
  for (const ConstrainedAdaptiveOctreeNode& node : asset.octree.nodes) {
    appendOctreeNode(&bytes, node);
  }
  const std::size_t directory_offset = bytes.size();
  bytes.resize(bytes.size() + records.size() * kBlockRecordBytes, 0);
  for (std::size_t i = 0; i < records.size(); ++i) {
    records[i].payload_offset = bytes.size();
    appendRaw(&bytes, payloads[i].data(), payloads[i].size());
  }
  std::vector<unsigned char> directory;
  for (const ConstrainedSDFBinBlockRecord& record : records) {
    appendBlockRecord(&directory, record);
  }
  if (directory.size() != records.size() * kBlockRecordBytes) {
    report.error_message = "internal constrained SDF directory size mismatch";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return false;
  }
  std::memcpy(
      bytes.data() + directory_offset, directory.data(), directory.size());
  const std::uint64_t final_size =
      static_cast<std::uint64_t>(bytes.size() + sizeof(std::uint64_t));
  std::memcpy(bytes.data() + file_size_offset, &final_size, sizeof(final_size));
  const std::uint64_t file_checksum = checksum(bytes.data(), bytes.size());
  append(&bytes, file_checksum);
  try {
    if (!path.parent_path().empty()) {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!output) {
      report.error_message = "failed to write constrained SDF file";
      if (report_out != nullptr) {
        *report_out = report;
      }
      return false;
    }
  } catch (const std::exception& exc) {
    report.error_message = exc.what();
    if (report_out != nullptr) {
      *report_out = report;
    }
    return false;
  }
  report.success = true;
  report.file_bytes = bytes.size();
  report.file_checksum = file_checksum;
  report.checksum_verified = true;
  report.octree_node_count = asset.octree.nodes.size();
  report.block_count = asset.blocks.size();
  if (report_out != nullptr) {
    *report_out = report;
  }
  return true;
}

ConstrainedSDFBinReader ConstrainedSDFBinReader::open(
    const std::filesystem::path& path,
    ConstrainedSDFBinReport* report_out) {
  ConstrainedSDFBinReader reader;
  ConstrainedSDFBinReport report;
  reader.path_ = path;
  std::string read_error;
  const std::vector<unsigned char> bytes = readFile(path, &read_error);
  if (bytes.size() < kMagicBytes + sizeof(std::uint64_t)) {
    report.error_message = read_error.empty()
        ? "constrained SDF file is truncated"
        : read_error;
    if (report_out != nullptr) {
      *report_out = report;
    }
    return reader;
  }
  std::uint64_t stored_checksum = 0;
  std::memcpy(
      &stored_checksum, bytes.data() + bytes.size() - sizeof(stored_checksum),
      sizeof(stored_checksum));
  const std::uint64_t computed_checksum =
      checksum(bytes.data(), bytes.size() - sizeof(stored_checksum));
  if (stored_checksum != computed_checksum) {
    report.error_message = "constrained SDF whole-file checksum mismatch";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return reader;
  }
  std::array<char, kMagicBytes> expected{};
  std::memcpy(
      expected.data(), ConstrainedSDFBin::magic(),
      std::min<std::size_t>(std::strlen(ConstrainedSDFBin::magic()), expected.size()));
  if (std::memcmp(bytes.data(), expected.data(), expected.size()) != 0) {
    report.error_message = "unsupported constrained SDF magic";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return reader;
  }
  std::size_t offset = kMagicBytes;
  std::uint32_t version = 0;
  std::uint32_t endian = 0;
  std::uint64_t file_size = 0;
  std::uint64_t node_count = 0;
  std::uint64_t block_count = 0;
  std::uint8_t strict_bound = 0;
  if (!read(bytes, &offset, &version) || !read(bytes, &offset, &endian) ||
      !read(bytes, &offset, &file_size) || !read(bytes, &offset, &node_count) ||
      !read(bytes, &offset, &block_count) ||
      !readAABB(bytes, &offset, &reader.bounds_) ||
      !read(bytes, &offset, &reader.max_zero_surface_abs_error_) ||
      !read(bytes, &offset, &strict_bound)) {
    report.error_message = "truncated constrained SDF header";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return reader;
  }
  offset += 7;
  if (version != kConstrainedSDFBinFormatVersion ||
      endian != kEndianMarker || file_size != bytes.size() ||
      node_count > 100000000ull || block_count > 10000000ull) {
    report.error_message = "unsupported or invalid constrained SDF header";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return reader;
  }
  reader.zero_surface_error_is_strict_bound_ = strict_bound != 0;
  reader.octree_nodes_.resize(static_cast<std::size_t>(node_count));
  for (ConstrainedAdaptiveOctreeNode& node : reader.octree_nodes_) {
    if (!readOctreeNode(bytes, &offset, &node)) {
      report.error_message = "truncated constrained SDF octree";
      if (report_out != nullptr) {
        *report_out = report;
      }
      return {};
    }
  }
  reader.blocks_.resize(static_cast<std::size_t>(block_count));
  for (ConstrainedSDFBinBlockRecord& block : reader.blocks_) {
    if (!readBlockRecord(bytes, &offset, &block) ||
        block.payload_offset > bytes.size() - sizeof(stored_checksum) ||
        block.payload_bytes > bytes.size() - sizeof(stored_checksum) -
            block.payload_offset) {
      report.error_message = "invalid constrained SDF block directory";
      if (report_out != nullptr) {
        *report_out = report;
      }
      return {};
    }
  }
  reader.valid_ = true;
  report.success = true;
  report.format_version = version;
  report.file_bytes = file_size;
  report.file_checksum = stored_checksum;
  report.checksum_verified = true;
  report.octree_node_count = reader.octree_nodes_.size();
  report.block_count = reader.blocks_.size();
  if (report_out != nullptr) {
    *report_out = report;
  }
  return reader;
}

bool ConstrainedSDFBinReader::loadBlock(
    std::size_t block_index,
    CompressedTensor3D* tensor,
    std::string* error_message) const {
  if (!valid_ || tensor == nullptr || block_index >= blocks_.size()) {
    if (error_message != nullptr) {
      *error_message = "invalid constrained SDF block request";
    }
    return false;
  }
  const ConstrainedSDFBinBlockRecord& record = blocks_[block_index];
  std::ifstream input(path_, std::ios::binary);
  if (!input) {
    if (error_message != nullptr) {
      *error_message = "failed to reopen constrained SDF file";
    }
    return false;
  }
  std::vector<unsigned char> payload(static_cast<std::size_t>(record.payload_bytes));
  input.seekg(static_cast<std::streamoff>(record.payload_offset));
  input.read(
      reinterpret_cast<char*>(payload.data()),
      static_cast<std::streamsize>(payload.size()));
  if (!input || checksum(payload.data(), payload.size()) != record.payload_checksum) {
    if (error_message != nullptr) {
      *error_message = "constrained SDF block payload checksum mismatch";
    }
    return false;
  }
  CompressedTensor3D out;
  out.requested_method = record.method;
  out.method = record.method;
  out.nx = record.nx;
  out.ny = record.ny;
  out.nz = record.nz;
  out.rank_x = record.rank_x;
  out.rank_y = record.rank_y;
  out.rank_z = record.rank_z;
  out.tolerance = record.tolerance;
  out.actual_max_abs_error = record.actual_max_abs_error;
  out.original_bytes = record.original_bytes;
  out.compressed_bytes = record.compressed_bytes;
  out.decoded_peak_bytes = record.decoded_peak_bytes;
  out.dense_fallback = record.method == TensorCompressionMethod::Dense;
  std::size_t offset = 0;
  if (!readVector(payload, &offset, &out.dense) ||
      !readVector(payload, &offset, &out.matrix_u) ||
      !readVector(payload, &offset, &out.matrix_s) ||
      !readVector(payload, &offset, &out.matrix_vt) ||
      !readVector(payload, &offset, &out.factor_x) ||
      !readVector(payload, &offset, &out.factor_y) ||
      !readVector(payload, &offset, &out.factor_z) ||
      !readVector(payload, &offset, &out.core) ||
      !readVector(payload, &offset, &out.tt_core_1) ||
      !readVector(payload, &offset, &out.tt_core_2) ||
      !readVector(payload, &offset, &out.tt_core_3) || offset != payload.size()) {
    if (error_message != nullptr) {
      *error_message = "invalid constrained SDF tensor payload";
    }
    return false;
  }
  *tensor = std::move(out);
  if (error_message != nullptr) {
    error_message->clear();
  }
  return true;
}

}  // namespace adasdf
