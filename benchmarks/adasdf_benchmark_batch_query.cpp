#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct BenchmarkRow {
  std::string backend;
  std::size_t num_points = 0;
  double total_ms = 0.0;
  double ns_per_query = 0.0;
  double queries_per_second = 0.0;
  double max_abs_phi_error = 0.0;
  double max_normal_error = 0.0;
  double speedup_vs_cpu = 0.0;
  bool cuda_available = false;
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
  std::ostringstream stream;
  stream << std::setprecision(10) << value;
  return stream.str();
}

void writeCsv(
    std::ostream& out,
    const std::vector<BenchmarkRow>& rows) {
  out << "backend,num_points,total_ms,ns_per_query,queries_per_second,"
         "max_abs_phi_error,max_normal_error,speedup_vs_cpu,cuda_available\n";
  for (const BenchmarkRow& row : rows) {
    out << row.backend << "," << row.num_points << ","
        << numberOrBlank(row.total_ms, row.skipped) << ","
        << numberOrBlank(row.ns_per_query, row.skipped) << ","
        << numberOrBlank(row.queries_per_second, row.skipped) << ","
        << numberOrBlank(row.max_abs_phi_error, row.skipped) << ","
        << numberOrBlank(row.max_normal_error, row.skipped) << ","
        << numberOrBlank(row.speedup_vs_cpu, row.skipped) << ","
        << (row.cuda_available ? "true" : "false") << "\n";
  }
}

void printSummary(const std::vector<BenchmarkRow>& rows) {
  std::cout << "N points | CPU total ms | CPU ns/query | GPU total ms | "
               "GPU ns/query | Speedup | Max phi error | Max normal error\n";
  for (const BenchmarkRow& cpu : rows) {
    if (cpu.backend != "cpu") {
      continue;
    }
    const BenchmarkRow* gpu = nullptr;
    for (const BenchmarkRow& row : rows) {
      if (row.backend == "cuda" && row.num_points == cpu.num_points) {
        gpu = &row;
        break;
      }
    }
    std::cout << cpu.num_points << " | " << cpu.total_ms << " | "
              << cpu.ns_per_query << " | ";
    if (!gpu || gpu->skipped) {
      std::cout << "SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED\n";
    } else {
      std::cout << gpu->total_ms << " | " << gpu->ns_per_query << " | "
                << gpu->speedup_vs_cpu << " | " << gpu->max_abs_phi_error
                << " | " << gpu->max_normal_error << "\n";
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string points_arg = "10000";
    std::string backend_arg = "cpu";
    std::filesystem::path output_path;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--points" && i + 1 < argc) {
        points_arg = argv[++i];
      } else if (arg == "--backend" && i + 1 < argc) {
        backend_arg = argv[++i];
      } else if (arg == "--out" && i + 1 < argc) {
        output_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        std::cout
            << "Usage: adasdf_benchmark_batch_query "
               "--points 10000,100000,1000000 --backend cpu,cuda "
               "[--out benchmark.csv]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    const std::vector<std::size_t> point_counts = parsePointCounts(points_arg);
    const std::vector<std::string> backends = splitList(backend_arg);
    const bool cuda_requested =
        std::find(backends.begin(), backends.end(), "cuda") != backends.end();
    const bool cpu_requested =
        std::find(backends.begin(), backends.end(), "cpu") != backends.end();
    if (!cpu_requested && !cuda_requested) {
      throw std::runtime_error("backend must include cpu and/or cuda");
    }

    const bool cuda_available = adasdf::CudaQueryBackend::isAvailable();
    if (cuda_requested && !cuda_available) {
      std::cout << "CUDA backend unavailable; skipping GPU benchmark.\n";
    }

    const auto model = adasdf::AnalyticSDFModel::createBox();
    std::vector<BenchmarkRow> rows;
    bool ran_cpu = false;
    bool ran_cuda = false;

    for (const std::size_t count : point_counts) {
      adasdf::PointCloudGeneratorOptions options;
      options.num_points = count;
      options.distribution = adasdf::BenchmarkPointDistribution::Mixed;
      options.seed = 1337;
      options.center = model->center();
      options.half_extent = model->halfExtent();
      const std::vector<adasdf::Vector3> points =
          adasdf::generateBenchmarkPoints(options);

      adasdf::BatchQueryStats cpu_stats;
      const adasdf::BatchQueryOutput cpu_output =
          adasdf::queryBatchCPU(*model, points, &cpu_stats);

      if (cpu_requested) {
        BenchmarkRow row;
        row.backend = "cpu";
        row.num_points = count;
        row.total_ms = cpu_stats.total_ms;
        row.ns_per_query = cpu_stats.ns_per_query;
        row.queries_per_second = cpu_stats.queries_per_second;
        row.speedup_vs_cpu = 1.0;
        row.cuda_available = cuda_available;
        rows.push_back(row);
        ran_cpu = true;
      }

      if (cuda_requested) {
        BenchmarkRow row;
        row.backend = "cuda";
        row.num_points = count;
        row.cuda_available = cuda_available;
        if (!cuda_available) {
          row.skipped = true;
          rows.push_back(row);
          continue;
        }

        adasdf::BatchQueryInput input;
        input.points = points;
        const auto t0 = std::chrono::steady_clock::now();
        const adasdf::BatchQueryOutput gpu_output =
            adasdf::CudaQueryBackend::queryAnalyticBox(*model, input);
        const auto t1 = std::chrono::steady_clock::now();
        row.total_ms =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        row.ns_per_query = row.total_ms * 1.0e6 / static_cast<double>(count);
        row.queries_per_second =
            row.total_ms > 0.0 ? static_cast<double>(count) * 1000.0 / row.total_ms
                               : 0.0;
        computeErrors(
            cpu_output,
            gpu_output,
            row.max_abs_phi_error,
            row.max_normal_error);
        row.speedup_vs_cpu =
            row.total_ms > 0.0 ? cpu_stats.total_ms / row.total_ms : 0.0;
        rows.push_back(row);
        ran_cuda = true;
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

    if (cuda_requested && !cpu_requested && !ran_cuda) {
      return 2;
    }
    return ran_cpu || ran_cuda ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "adasdf_benchmark_batch_query failed: " << error.what()
              << "\n";
    return 1;
  }
}
