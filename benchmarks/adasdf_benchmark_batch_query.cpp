#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct BenchmarkRow {
  std::string query_backend;
  std::string expansion_mode;
  std::string selected_blocks;
  std::size_t num_points = 0;
  double expanded_memory_mb = 0.0;
  double gpu_resident_memory_mb = 0.0;
  double setup_ms = 0.0;
  double query_kernel_ms = 0.0;
  double query_total_ms = 0.0;
  double ns_per_query = 0.0;
  double queries_per_second = 0.0;
  std::size_t fallback_count = 0;
  double max_abs_phi_error = 0.0;
  double max_normal_error = 0.0;
  bool cuda_available = false;
  std::string status = "ok";
  std::string error_message;
  bool skipped = false;
};

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(item);
    }
  }
  return values;
}

std::vector<std::size_t> parsePointCounts(const std::string& text) {
  std::vector<std::size_t> values;
  for (const std::string& item : splitList(text)) {
    const auto count = static_cast<std::size_t>(std::stoull(item));
    if (count == 0) {
      throw std::runtime_error("point counts must be positive");
    }
    values.push_back(count);
  }
  if (values.empty()) {
    throw std::runtime_error("at least one point count is required");
  }
  return values;
}

adasdf::QueryBackend parseBackend(const std::string& text) {
  if (text == "cpu") {
    return adasdf::QueryBackend::CPU;
  }
  if (text == "cuda") {
    return adasdf::QueryBackend::CUDA;
  }
  if (text == "auto") {
    return adasdf::QueryBackend::Auto;
  }
  throw std::runtime_error("query backend must be cpu, cuda, or auto");
}

adasdf::QueryExpansionMode parseExpansion(const std::string& text) {
  if (text == "none") {
    return adasdf::QueryExpansionMode::None;
  }
  if (text == "global") {
    return adasdf::QueryExpansionMode::Global;
  }
  if (text == "block") {
    return adasdf::QueryExpansionMode::Block;
  }
  if (text == "auto") {
    return adasdf::QueryExpansionMode::Auto;
  }
  throw std::runtime_error("expansion must be none, global, block, or auto");
}

adasdf::BlockSelection parseBlocks(const std::string& text) {
  if (text.empty() || text == "all") {
    return adasdf::BlockSelection::all();
  }
  std::vector<int> ids;
  for (const std::string& item : splitList(text)) {
    ids.push_back(std::stoi(item));
  }
  return adasdf::BlockSelection::selected(std::move(ids));
}

double vectorError(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void computeErrors(
    const adasdf::BatchQueryOutput& reference,
    const adasdf::BatchQueryOutput& candidate,
    double& max_phi_error,
    double& max_normal_error) {
  max_phi_error = 0.0;
  max_normal_error = 0.0;
  const std::size_t count = std::min(
      reference.signed_distances.size(),
      candidate.signed_distances.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (!std::isfinite(candidate.signed_distances[i])) {
      max_phi_error = std::numeric_limits<double>::infinity();
      max_normal_error = std::numeric_limits<double>::infinity();
      return;
    }
    max_phi_error = std::max(
        max_phi_error,
        std::abs(reference.signed_distances[i] - candidate.signed_distances[i]));
    max_normal_error = std::max(
        max_normal_error,
        vectorError(reference.normals[i], candidate.normals[i]));
  }
}

std::string numberOrBlank(double value, bool blank) {
  if (blank) {
    return "";
  }
  if (!std::isfinite(value)) {
    return "inf";
  }
  std::ostringstream stream;
  stream << std::setprecision(10) << value;
  return stream.str();
}

std::string numberOrNA(double value, bool na) {
  if (na) {
    return "NA";
  }
  return numberOrBlank(value, false);
}

std::string csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }
  escaped += "\"";
  return escaped;
}

void writeCsv(std::ostream& out, const std::vector<BenchmarkRow>& rows) {
  out << "query_backend,expansion_mode,selected_blocks,num_points,"
         "expanded_memory_mb,gpu_resident_memory_mb,setup_ms,query_kernel_ms,"
         "query_total_ms,ns_per_query,queries_per_second,fallback_count,"
         "max_abs_phi_error,max_normal_error,cuda_available,status,error_message\n";
  for (const BenchmarkRow& row : rows) {
    const bool cpu_kernel_na = row.query_backend != "cuda" || row.skipped;
    out << csvField(row.query_backend) << "," << csvField(row.expansion_mode) << ","
        << csvField(row.selected_blocks) << "," << row.num_points << ","
        << numberOrBlank(row.expanded_memory_mb, row.skipped) << ","
        << numberOrBlank(row.gpu_resident_memory_mb, row.skipped) << ","
        << numberOrBlank(row.setup_ms, row.skipped) << ","
        << numberOrNA(row.query_kernel_ms, cpu_kernel_na) << ","
        << numberOrBlank(row.query_total_ms, row.skipped) << ","
        << numberOrBlank(row.ns_per_query, row.skipped) << ","
        << numberOrBlank(row.queries_per_second, row.skipped) << ","
        << row.fallback_count << ","
        << numberOrBlank(row.max_abs_phi_error, row.skipped) << ","
        << numberOrBlank(row.max_normal_error, row.skipped) << ","
        << (row.cuda_available ? "true" : "false") << ","
        << csvField(row.status) << "," << csvField(row.error_message) << "\n";
  }
}

adasdf::AABB blockDomain(
    const adasdf::SDFModel& model,
    const adasdf::BlockSelection& selection) {
  if (selection.use_all_blocks || model.blockMetadata().empty()) {
    return model.boundingBox();
  }

  adasdf::AABB domain;
  domain.valid = false;
  for (const adasdf::SDFBlockMetadata& block : model.blockMetadata()) {
    if (std::find(
            selection.block_ids.begin(),
            selection.block_ids.end(),
            static_cast<int>(block.block_id)) == selection.block_ids.end()) {
      continue;
    }
    if (!domain.valid) {
      domain.min = block.local_min;
      domain.max = block.local_max;
      domain.valid = true;
    } else {
      domain.min.x = std::min(domain.min.x, block.local_min.x);
      domain.min.y = std::min(domain.min.y, block.local_min.y);
      domain.min.z = std::min(domain.min.z, block.local_min.z);
      domain.max.x = std::max(domain.max.x, block.local_max.x);
      domain.max.y = std::max(domain.max.y, block.local_max.y);
      domain.max.z = std::max(domain.max.z, block.local_max.z);
    }
  }
  if (!domain.valid) {
    throw std::runtime_error("selected benchmark blocks do not exist");
  }
  return domain;
}

std::vector<adasdf::Vector3> makePoints(
    const adasdf::SDFModel& model,
    const adasdf::BlockSelection& selection,
    std::size_t count) {
  const adasdf::AABB domain = blockDomain(model, selection);
  adasdf::PointCloudGeneratorOptions options;
  options.num_points = count;
  options.distribution = adasdf::BenchmarkPointDistribution::UniformBoxVolume;
  options.seed = 1337;
  options.center = 0.5 * (domain.min + domain.max);
  options.half_extent = 0.5 * (domain.max - domain.min);
  options.volume_scale = 0.9;
  return adasdf::generateBenchmarkPoints(options);
}

void printSummary(const std::vector<BenchmarkRow>& rows) {
  std::cout
      << "backend | expansion | blocks | points | setup ms | query total ms | "
         "kernel ms | ns/query | qps | fallback | max phi error | max normal error | status\n";
  for (const BenchmarkRow& row : rows) {
    std::cout << row.query_backend << " | " << row.expansion_mode << " | "
              << row.selected_blocks << " | " << row.num_points << " | ";
    if (row.skipped) {
      std::cout << "SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | "
                << row.fallback_count << " | SKIPPED | SKIPPED | "
                << row.status << "\n";
      continue;
    }
    std::cout << row.setup_ms << " | " << row.query_total_ms << " | ";
    if (row.query_backend == "cuda") {
      std::cout << row.query_kernel_ms;
    } else {
      std::cout << "NA";
    }
    std::cout << " | " << row.ns_per_query << " | "
              << row.queries_per_second << " | " << row.fallback_count
              << " | " << row.max_abs_phi_error << " | "
              << row.max_normal_error << " | " << row.status << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string points_arg = "10000";
    std::string backend_arg = "cpu";
    std::string expansion_arg = "none";
    std::string blocks_arg = "all";
    bool expansion_was_set = false;
    int global_resolution = 64;
    int block_resolution = 32;
    bool keep_resident = true;
    std::filesystem::path output_path;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--points" && i + 1 < argc) {
        points_arg = argv[++i];
      } else if ((arg == "--query-backend" || arg == "--backend") &&
                 i + 1 < argc) {
        backend_arg = argv[++i];
      } else if (arg == "--expansion" && i + 1 < argc) {
        expansion_arg = argv[++i];
        expansion_was_set = true;
      } else if (arg == "--blocks" && i + 1 < argc) {
        blocks_arg = argv[++i];
      } else if (arg == "--global-resolution" && i + 1 < argc) {
        global_resolution = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && i + 1 < argc) {
        block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--keep-resident") {
        keep_resident = true;
      } else if (arg == "--out" && i + 1 < argc) {
        output_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        std::cout
            << "Usage: adasdf_benchmark_batch_query --points 10000,100000,1000000 "
               "--query-backend cpu|cuda --expansion none|global|block "
               "[--blocks all|0,1,2] [--global-resolution 64] "
               "[--block-resolution 32] [--keep-resident] [--out benchmark.csv]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    const std::vector<std::size_t> point_counts = parsePointCounts(points_arg);
    const std::vector<std::string> backend_names = splitList(backend_arg);
    const adasdf::BlockSelection block_selection = parseBlocks(blocks_arg);
    const bool cuda_available = adasdf::CudaQueryBackend::isAvailable();

    adasdf::DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = false;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);
    if (!build.model) {
      throw std::runtime_error("failed to create demo adaptive benchmark model");
    }
    std::shared_ptr<adasdf::SDFModel> model = build.model;

    std::vector<BenchmarkRow> rows;
    for (const std::string& backend_name : backend_names) {
      const adasdf::QueryBackend backend = parseBackend(backend_name);
      std::string effective_expansion_arg = expansion_arg;
      if (!expansion_was_set) {
        effective_expansion_arg =
            backend == adasdf::QueryBackend::CUDA ? "global" : "none";
      }
      const adasdf::QueryExpansionMode expansion =
          parseExpansion(effective_expansion_arg);

      for (const std::size_t count : point_counts) {
        BenchmarkRow row;
        row.query_backend = adasdf::toString(backend);
        row.expansion_mode = adasdf::toString(expansion);
        row.selected_blocks = adasdf::blockSelectionString(block_selection);
        row.num_points = count;
        row.cuda_available = cuda_available;

        if (backend == adasdf::QueryBackend::CUDA && !cuda_available) {
          row.status = "skipped";
          row.error_message = "CUDA backend unavailable";
          row.skipped = true;
          rows.push_back(row);
          continue;
        }

        try {
          adasdf::QueryModeConfig config;
          config.backend = backend;
          config.expansion = expansion;
          config.block_selection = block_selection;
          config.keep_expanded_data_resident = keep_resident;
          config.allow_fallback_to_cpu = backend != adasdf::QueryBackend::CUDA;

          adasdf::ExpansionOptions expansion_options;
          expansion_options.expansion = expansion;
          expansion_options.block_selection = block_selection;
          expansion_options.global_resolution = global_resolution;
          expansion_options.block_resolution = block_resolution;
          expansion_options.padding = 0.0;

          const std::vector<adasdf::Vector3> points =
              makePoints(*model, block_selection, count);
          const adasdf::BatchQueryOutput reference =
              adasdf::queryBatchCPU(*model, points);

          adasdf::QueryEngine engine(model, config, expansion_options);
          const auto setup0 = std::chrono::steady_clock::now();
          if (!engine.prepare()) {
            throw std::runtime_error("QueryEngine prepare failed");
          }
          const auto setup1 = std::chrono::steady_clock::now();
          adasdf::BatchQueryOutput output = engine.queryBatch(points);

          const adasdf::QueryEngineStats& stats = engine.stats();
          row.setup_ms =
              std::chrono::duration<double, std::milli>(setup1 - setup0).count();
          if (stats.setup_ms > 0.0) {
            row.setup_ms = stats.setup_ms;
          }
          row.expanded_memory_mb =
              static_cast<double>(stats.expanded_memory_bytes) /
              (1024.0 * 1024.0);
          row.gpu_resident_memory_mb =
              static_cast<double>(stats.gpu_resident_memory_bytes) /
              (1024.0 * 1024.0);
          row.query_kernel_ms = stats.query_kernel_ms;
          row.query_total_ms = stats.query_total_ms;
          row.fallback_count = stats.fallback_count;
          row.ns_per_query =
              count > 0 ? row.query_total_ms * 1.0e6 / static_cast<double>(count)
                        : 0.0;
          row.queries_per_second =
              row.query_total_ms > 0.0
                  ? static_cast<double>(count) * 1000.0 / row.query_total_ms
                  : 0.0;
          computeErrors(
              reference,
              output,
              row.max_abs_phi_error,
              row.max_normal_error);
          rows.push_back(row);
        } catch (const std::exception& error) {
          row.status = "failed";
          row.error_message = error.what();
          rows.push_back(row);
        }
      }
    }

    writeCsv(std::cout, rows);
    printSummary(rows);

    if (!output_path.empty()) {
      std::ofstream file(output_path);
      if (!file) {
        throw std::runtime_error("failed to open benchmark output CSV");
      }
      writeCsv(file, rows);
    }

    const bool any_ok = std::any_of(
        rows.begin(),
        rows.end(),
        [](const BenchmarkRow& row) { return row.status == "ok"; });
    return any_ok ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "adasdf_benchmark_batch_query failed: " << error.what()
              << "\n";
    return 1;
  }
}
