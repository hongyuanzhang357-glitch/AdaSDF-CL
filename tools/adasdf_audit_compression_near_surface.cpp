#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct CliOptions {
  std::filesystem::path dense;
  std::filesystem::path compressed;
  std::filesystem::path stl;
  double near_band = 0.01;
  std::size_t surface_samples = 3000;
  std::vector<double> offsets = {-0.01, -0.005, -0.002, 0.0,
                                 0.002, 0.005, 0.01};
  std::filesystem::path json;
  std::filesystem::path csv;
  std::filesystem::path report;
};

struct SampleRow {
  std::size_t sample_index = 0;
  int triangle_index = -1;
  double offset = 0.0;
  adasdf::Vector3 point;
  double reference_phi = 0.0;
  double dense_phi = 0.0;
  double compressed_phi = 0.0;
  double dense_ref_abs_error = 0.0;
  double compressed_ref_abs_error = 0.0;
  double compressed_dense_abs_error = 0.0;
  bool dense_sign_mismatch = false;
  bool compressed_sign_mismatch = false;
  bool compressed_vs_dense_sign_mismatch = false;
  bool near_zero_compression_sample = false;
  int dense_block_id = -1;
  int dense_block_level = -1;
  int compressed_block_id = -1;
  int compressed_block_level = -1;
  int compressed_rank = 0;
  bool compressed_dense_fallback = false;
};

struct BlockAggregate {
  int block_id = -1;
  int level = -1;
  int rank = 0;
  bool dense_fallback = false;
  std::size_t sample_count = 0;
  std::size_t sign_flip_count = 0;
  double max_abs_error = 0.0;
  double p95_abs_error = 0.0;
  std::vector<double> errors;
};

struct AuditResult {
  std::size_t sample_count = 0;
  std::size_t near_surface_sample_count = 0;
  double dense_ref_p95_abs_error = 0.0;
  double compressed_ref_p95_abs_error = 0.0;
  double compressed_dense_p95_abs_error = 0.0;
  std::size_t dense_sign_mismatch_count = 0;
  std::size_t compressed_sign_mismatch_count = 0;
  std::size_t compressed_vs_dense_sign_mismatch_count = 0;
  std::size_t near_zero_compression_sign_flip_count = 0;
  double near_zero_compression_p95_error = 0.0;
  std::vector<SampleRow> rows;
  std::vector<BlockAggregate> worst_blocks_by_compression_error;
  std::vector<BlockAggregate> worst_blocks_by_sign_flip;
};

void usage() {
  std::cerr
      << "Usage: adasdf_audit_compression_near_surface "
         "--dense dense_model.sdfbin --compressed compressed_model.sdfbin "
         "--stl original.stl [--near-band 0.01] "
         "[--surface-samples 3000] [--offsets a,b,c] "
         "[--json out.json] [--csv out.csv] [--report out.md]\n";
}

std::vector<double> parseDoubleList(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stod(item));
    }
  }
  return values;
}

bool parseArgs(int argc, char** argv, CliOptions* options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto need_value = [&](const char* name) {
      if (i + 1 >= argc) {
        std::cerr << name << " requires a value\n";
        return false;
      }
      return true;
    };
    if (arg == "--dense" && need_value("--dense")) {
      options->dense = argv[++i];
    } else if (arg == "--compressed" && need_value("--compressed")) {
      options->compressed = argv[++i];
    } else if (arg == "--stl" && need_value("--stl")) {
      options->stl = argv[++i];
    } else if (arg == "--near-band" && need_value("--near-band")) {
      options->near_band = std::stod(argv[++i]);
    } else if (arg == "--surface-samples" &&
               need_value("--surface-samples")) {
      options->surface_samples =
          static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--offsets" && need_value("--offsets")) {
      options->offsets = parseDoubleList(argv[++i]);
    } else if (arg == "--json" && need_value("--json")) {
      options->json = argv[++i];
    } else if (arg == "--csv" && need_value("--csv")) {
      options->csv = argv[++i];
    } else if (arg == "--report" && need_value("--report")) {
      options->report = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    }
  }
  return true;
}

bool validate(const CliOptions& options) {
  if (options.dense.empty() || options.compressed.empty() ||
      options.stl.empty()) {
    std::cerr << "--dense, --compressed, and --stl are required\n";
    return false;
  }
  if (!std::filesystem::exists(options.dense)) {
    std::cerr << "dense SDF file not found: " << options.dense.string()
              << "\n";
    return false;
  }
  if (!std::filesystem::exists(options.compressed)) {
    std::cerr << "compressed SDF file not found: "
              << options.compressed.string() << "\n";
    return false;
  }
  if (!std::filesystem::exists(options.stl)) {
    std::cerr << "STL file not found: " << options.stl.string() << "\n";
    return false;
  }
  if (!(options.near_band > 0.0) || options.surface_samples == 0 ||
      options.offsets.empty()) {
    std::cerr << "near-band, surface-samples, and offsets must be valid\n";
    return false;
  }
  return true;
}

void createParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

double percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const double clamped = std::clamp(q, 0.0, 1.0);
  const std::size_t index = static_cast<std::size_t>(
      std::round(clamped * static_cast<double>(values.size() - 1)));
  return values[index];
}

bool signMismatch(double a, double b) {
  if (std::abs(a) <= 1.0e-12 || std::abs(b) <= 1.0e-12) {
    return false;
  }
  return (a < 0.0) != (b < 0.0);
}

void appendBlockAggregate(
    const std::shared_ptr<adasdf::CompressedAdaptiveBlockSDFModel>& compressed,
    const SampleRow& row,
    std::map<int, BlockAggregate>* blocks) {
  const int key = row.compressed_block_id;
  BlockAggregate& aggregate = (*blocks)[key];
  if (aggregate.sample_count == 0) {
    aggregate.block_id = row.compressed_block_id;
    aggregate.level = row.compressed_block_level;
    aggregate.rank = row.compressed_rank;
    aggregate.dense_fallback = row.compressed_dense_fallback;
  }
  if (compressed && row.compressed_block_id >= 0) {
    const int block_index = compressed->findContainingBlock(row.point);
    if (block_index >= 0) {
      const adasdf::CompressedSDFBlock& block =
          compressed->compressedBlockSet().blocks[
              static_cast<std::size_t>(block_index)];
      aggregate.level = block.level;
      aggregate.rank =
          block.method == adasdf::BlockCompressionMethod::MatrixSVD
              ? block.svd.rank
              : 0;
      aggregate.dense_fallback =
          block.method == adasdf::BlockCompressionMethod::DenseFallback;
    }
  }
  ++aggregate.sample_count;
  if (row.compressed_vs_dense_sign_mismatch) {
    ++aggregate.sign_flip_count;
  }
  aggregate.max_abs_error =
      std::max(aggregate.max_abs_error, row.compressed_dense_abs_error);
  aggregate.errors.push_back(row.compressed_dense_abs_error);
}

std::string jsonEscape(const std::string& text) {
  std::ostringstream out;
  for (char c : text) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << c;
        break;
    }
  }
  return out.str();
}

void writeBlockArray(
    std::ostream& out,
    const std::vector<BlockAggregate>& blocks,
    int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << "[\n";
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    const BlockAggregate& block = blocks[i];
    out << pad << "  {"
        << "\"block_id\":" << block.block_id << ","
        << "\"level\":" << block.level << ","
        << "\"compressed_rank\":" << block.rank << ","
        << "\"dense_fallback\":"
        << (block.dense_fallback ? "true" : "false") << ","
        << "\"sample_count\":" << block.sample_count << ","
        << "\"sign_flip_count\":" << block.sign_flip_count << ","
        << "\"p95_abs_error\":" << block.p95_abs_error << ","
        << "\"max_abs_error\":" << block.max_abs_error << "}";
    if (i + 1 < blocks.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << pad << "]";
}

bool writeJson(const std::filesystem::path& path, const AuditResult& result) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema\":\"adasdf.compression_near_surface_audit.v1\",\n";
  out << "  \"sample_count\":" << result.sample_count << ",\n";
  out << "  \"near_surface_sample_count\":"
      << result.near_surface_sample_count << ",\n";
  out << "  \"dense_ref_p95_abs_error\":"
      << result.dense_ref_p95_abs_error << ",\n";
  out << "  \"compressed_ref_p95_abs_error\":"
      << result.compressed_ref_p95_abs_error << ",\n";
  out << "  \"compressed_dense_p95_abs_error\":"
      << result.compressed_dense_p95_abs_error << ",\n";
  out << "  \"dense_sign_mismatch_count\":"
      << result.dense_sign_mismatch_count << ",\n";
  out << "  \"compressed_sign_mismatch_count\":"
      << result.compressed_sign_mismatch_count << ",\n";
  out << "  \"compressed_vs_dense_sign_mismatch_count\":"
      << result.compressed_vs_dense_sign_mismatch_count << ",\n";
  out << "  \"near_zero_compression_sign_flip_count\":"
      << result.near_zero_compression_sign_flip_count << ",\n";
  out << "  \"near_zero_compression_p95_error\":"
      << result.near_zero_compression_p95_error << ",\n";
  out << "  \"worst_blocks_by_compression_error\":";
  writeBlockArray(out, result.worst_blocks_by_compression_error, 2);
  out << ",\n";
  out << "  \"worst_blocks_by_sign_flip\":";
  writeBlockArray(out, result.worst_blocks_by_sign_flip, 2);
  out << "\n}\n";
  return true;
}

bool writeCsv(const std::filesystem::path& path, const AuditResult& result) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(17);
  out << "sample_index,triangle_index,offset,x,y,z,reference_phi,dense_phi,"
         "compressed_phi,dense_ref_abs_error,compressed_ref_abs_error,"
         "compressed_dense_abs_error,dense_sign_mismatch,"
         "compressed_sign_mismatch,compressed_vs_dense_sign_mismatch,"
         "near_zero_compression_sample,dense_block_id,dense_block_level,"
         "compressed_block_id,compressed_block_level,compressed_rank,"
         "compressed_dense_fallback\n";
  for (const SampleRow& row : result.rows) {
    out << row.sample_index << "," << row.triangle_index << ","
        << row.offset << "," << row.point.x << "," << row.point.y << ","
        << row.point.z << "," << row.reference_phi << ","
        << row.dense_phi << "," << row.compressed_phi << ","
        << row.dense_ref_abs_error << ","
        << row.compressed_ref_abs_error << ","
        << row.compressed_dense_abs_error << ","
        << (row.dense_sign_mismatch ? 1 : 0) << ","
        << (row.compressed_sign_mismatch ? 1 : 0) << ","
        << (row.compressed_vs_dense_sign_mismatch ? 1 : 0) << ","
        << (row.near_zero_compression_sample ? 1 : 0) << ","
        << row.dense_block_id << "," << row.dense_block_level << ","
        << row.compressed_block_id << "," << row.compressed_block_level
        << "," << row.compressed_rank << ","
        << (row.compressed_dense_fallback ? 1 : 0) << "\n";
  }
  return true;
}

bool writeMarkdown(const std::filesystem::path& path, const AuditResult& result) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(12);
  out << "# Compression Near-Surface Audit\n\n";
  out << "| Metric | Value |\n";
  out << "|---|---:|\n";
  out << "| sample_count | " << result.sample_count << " |\n";
  out << "| near_surface_sample_count | "
      << result.near_surface_sample_count << " |\n";
  out << "| dense_ref_p95_abs_error | "
      << result.dense_ref_p95_abs_error << " |\n";
  out << "| compressed_ref_p95_abs_error | "
      << result.compressed_ref_p95_abs_error << " |\n";
  out << "| compressed_dense_p95_abs_error | "
      << result.compressed_dense_p95_abs_error << " |\n";
  out << "| dense_sign_mismatch_count | "
      << result.dense_sign_mismatch_count << " |\n";
  out << "| compressed_sign_mismatch_count | "
      << result.compressed_sign_mismatch_count << " |\n";
  out << "| compressed_vs_dense_sign_mismatch_count | "
      << result.compressed_vs_dense_sign_mismatch_count << " |\n";
  out << "| near_zero_compression_sign_flip_count | "
      << result.near_zero_compression_sign_flip_count << " |\n";
  out << "| near_zero_compression_p95_error | "
      << result.near_zero_compression_p95_error << " |\n\n";
  out << "## Worst Blocks By Compression Error\n\n";
  out << "| block_id | level | rank | dense_fallback | samples | sign_flips | p95 | max |\n";
  out << "|---:|---:|---:|---|---:|---:|---:|---:|\n";
  for (const BlockAggregate& block : result.worst_blocks_by_compression_error) {
    out << "| " << block.block_id << " | " << block.level << " | "
        << block.rank << " | " << (block.dense_fallback ? "yes" : "no")
        << " | " << block.sample_count << " | "
        << block.sign_flip_count << " | " << block.p95_abs_error
        << " | " << block.max_abs_error << " |\n";
  }
  out << "\n## Worst Blocks By Sign Flip\n\n";
  out << "| block_id | level | rank | dense_fallback | samples | sign_flips | p95 | max |\n";
  out << "|---:|---:|---:|---|---:|---:|---:|---:|\n";
  for (const BlockAggregate& block : result.worst_blocks_by_sign_flip) {
    out << "| " << block.block_id << " | " << block.level << " | "
        << block.rank << " | " << (block.dense_fallback ? "yes" : "no")
        << " | " << block.sample_count << " | "
        << block.sign_flip_count << " | " << block.p95_abs_error
        << " | " << block.max_abs_error << " |\n";
  }
  return true;
}

void fillDenseBlockInfo(
    const std::shared_ptr<adasdf::AdaptiveBlockSDFModel>& dense,
    SampleRow* row) {
  if (!dense || row == nullptr) {
    return;
  }
  const int index = dense->findContainingBlock(row->point);
  if (index < 0) {
    return;
  }
  const adasdf::AdaptiveSDFBlock& block =
      dense->blockSet().blocks[static_cast<std::size_t>(index)];
  row->dense_block_id = block.block_id;
  row->dense_block_level = block.level;
}

void fillCompressedBlockInfo(
    const std::shared_ptr<adasdf::CompressedAdaptiveBlockSDFModel>& compressed,
    SampleRow* row) {
  if (!compressed || row == nullptr) {
    return;
  }
  const int index = compressed->findContainingBlock(row->point);
  if (index < 0) {
    return;
  }
  const adasdf::CompressedSDFBlock& block =
      compressed->compressedBlockSet().blocks[static_cast<std::size_t>(index)];
  row->compressed_block_id = block.block_id;
  row->compressed_block_level = block.level;
  row->compressed_rank =
      block.method == adasdf::BlockCompressionMethod::MatrixSVD
          ? block.svd.rank
          : 0;
  row->compressed_dense_fallback =
      block.method == adasdf::BlockCompressionMethod::DenseFallback;
}

std::vector<BlockAggregate> sortedBlocks(
    std::map<int, BlockAggregate> blocks,
    bool by_sign_flip) {
  std::vector<BlockAggregate> out;
  out.reserve(blocks.size());
  for (auto& item : blocks) {
    item.second.p95_abs_error = percentile(item.second.errors, 0.95);
    out.push_back(std::move(item.second));
  }
  std::sort(
      out.begin(),
      out.end(),
      [by_sign_flip](const BlockAggregate& a, const BlockAggregate& b) {
        if (by_sign_flip && a.sign_flip_count != b.sign_flip_count) {
          return a.sign_flip_count > b.sign_flip_count;
        }
        if (a.p95_abs_error != b.p95_abs_error) {
          return a.p95_abs_error > b.p95_abs_error;
        }
        if (a.max_abs_error != b.max_abs_error) {
          return a.max_abs_error > b.max_abs_error;
        }
        return a.block_id < b.block_id;
      });
  if (out.size() > 10) {
    out.resize(10);
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    CliOptions cli;
    if (!parseArgs(argc, argv, &cli) || !validate(cli)) {
      usage();
      return 2;
    }

    const adasdf::STLReadResult read = adasdf::STLReader::read(cli.stl.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }
    std::shared_ptr<adasdf::SDFModel> dense_model =
        adasdf::SDFBinReader::read(cli.dense);
    std::shared_ptr<adasdf::SDFModel> compressed_model =
        adasdf::SDFBinReader::read(cli.compressed);
    if (!dense_model || !dense_model->isValid() ||
        !dense_model->queryBackendAvailable()) {
      std::cerr << "dense SDF model does not have a CPU query backend\n";
      return 1;
    }
    if (!compressed_model || !compressed_model->isValid() ||
        !compressed_model->queryBackendAvailable()) {
      std::cerr << "compressed SDF model does not have a CPU query backend\n";
      return 1;
    }
    auto dense_adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(dense_model);
    auto compressed_adaptive =
        std::dynamic_pointer_cast<adasdf::CompressedAdaptiveBlockSDFModel>(
            compressed_model);

    adasdf::NearSurfaceSampleOptions sample_options;
    sample_options.surface_sample_count = cli.surface_samples;
    sample_options.offsets = cli.offsets;
    const adasdf::NearSurfaceSampleSet samples =
        adasdf::NearSurfaceSampleGenerator::generate(read.mesh, sample_options);
    if (samples.samples.empty()) {
      std::cerr << "near-surface sample generation produced no samples\n";
      return 1;
    }

    adasdf::BVHSDFSamplerOptions reference_options;
    reference_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    reference_options.signed_distance = true;
    reference_options.fallback_to_bruteforce_sign = true;
    adasdf::BVHSDFSampler reference_sampler;
    if (!reference_sampler.reset(read.mesh, reference_options)) {
      std::cerr << "failed to initialize BVH reference sampler\n";
      return 1;
    }

    AuditResult result;
    result.sample_count = samples.samples.size();
    std::vector<double> dense_ref_errors;
    std::vector<double> compressed_ref_errors;
    std::vector<double> compressed_dense_errors;
    std::vector<double> near_zero_compression_errors;
    std::map<int, BlockAggregate> block_aggregates;

    for (std::size_t i = 0; i < samples.samples.size(); ++i) {
      const adasdf::NearSurfaceSample& sample = samples.samples[i];
      const adasdf::BVHSDFSampleResult reference =
          reference_sampler.sample(sample.point);
      if (!reference.success || !std::isfinite(reference.phi)) {
        continue;
      }
      if (std::abs(reference.phi) > cli.near_band) {
        continue;
      }
      SampleRow row;
      row.sample_index = i;
      row.triangle_index = sample.triangle_index;
      row.offset = sample.offset;
      row.point = sample.point;
      row.reference_phi = reference.phi;
      row.dense_phi = dense_model->sampleDistance(sample.point);
      row.compressed_phi = compressed_model->sampleDistance(sample.point);
      row.dense_ref_abs_error =
          std::abs(row.dense_phi - row.reference_phi);
      row.compressed_ref_abs_error =
          std::abs(row.compressed_phi - row.reference_phi);
      row.compressed_dense_abs_error =
          std::abs(row.compressed_phi - row.dense_phi);
      row.dense_sign_mismatch =
          signMismatch(row.dense_phi, row.reference_phi);
      row.compressed_sign_mismatch =
          signMismatch(row.compressed_phi, row.reference_phi);
      row.compressed_vs_dense_sign_mismatch =
          signMismatch(row.compressed_phi, row.dense_phi);
      row.near_zero_compression_sample =
          std::abs(row.dense_phi) <= cli.near_band;
      fillDenseBlockInfo(dense_adaptive, &row);
      fillCompressedBlockInfo(compressed_adaptive, &row);

      dense_ref_errors.push_back(row.dense_ref_abs_error);
      compressed_ref_errors.push_back(row.compressed_ref_abs_error);
      compressed_dense_errors.push_back(row.compressed_dense_abs_error);
      if (row.dense_sign_mismatch) {
        ++result.dense_sign_mismatch_count;
      }
      if (row.compressed_sign_mismatch) {
        ++result.compressed_sign_mismatch_count;
      }
      if (row.compressed_vs_dense_sign_mismatch) {
        ++result.compressed_vs_dense_sign_mismatch_count;
      }
      if (row.near_zero_compression_sample) {
        near_zero_compression_errors.push_back(
            row.compressed_dense_abs_error);
        if (row.compressed_vs_dense_sign_mismatch) {
          ++result.near_zero_compression_sign_flip_count;
        }
      }
      appendBlockAggregate(compressed_adaptive, row, &block_aggregates);
      result.rows.push_back(row);
    }

    result.near_surface_sample_count = result.rows.size();
    result.dense_ref_p95_abs_error = percentile(dense_ref_errors, 0.95);
    result.compressed_ref_p95_abs_error =
        percentile(compressed_ref_errors, 0.95);
    result.compressed_dense_p95_abs_error =
        percentile(compressed_dense_errors, 0.95);
    result.near_zero_compression_p95_error =
        percentile(near_zero_compression_errors, 0.95);
    result.worst_blocks_by_compression_error =
        sortedBlocks(block_aggregates, false);
    result.worst_blocks_by_sign_flip =
        sortedBlocks(block_aggregates, true);

    if (!cli.json.empty() && !writeJson(cli.json, result)) {
      std::cerr << "failed to write JSON: " << cli.json.string() << "\n";
      return 1;
    }
    if (!cli.csv.empty() && !writeCsv(cli.csv, result)) {
      std::cerr << "failed to write CSV: " << cli.csv.string() << "\n";
      return 1;
    }
    if (!cli.report.empty() && !writeMarkdown(cli.report, result)) {
      std::cerr << "failed to write report: " << cli.report.string() << "\n";
      return 1;
    }

    std::cout << "compression near-surface audit completed\n";
    std::cout << "sample_count=" << result.sample_count << "\n";
    std::cout << "near_surface_sample_count="
              << result.near_surface_sample_count << "\n";
    std::cout << "dense_ref_p95_abs_error="
              << result.dense_ref_p95_abs_error << "\n";
    std::cout << "compressed_ref_p95_abs_error="
              << result.compressed_ref_p95_abs_error << "\n";
    std::cout << "compressed_dense_p95_abs_error="
              << result.compressed_dense_p95_abs_error << "\n";
    std::cout << "compressed_vs_dense_sign_mismatch_count="
              << result.compressed_vs_dense_sign_mismatch_count << "\n";
    std::cout << "near_zero_compression_sign_flip_count="
              << result.near_zero_compression_sign_flip_count << "\n";
    std::cout << "near_zero_compression_p95_error="
              << result.near_zero_compression_p95_error << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_audit_compression_near_surface failed: "
              << exc.what() << "\n";
    return 1;
  }
}
