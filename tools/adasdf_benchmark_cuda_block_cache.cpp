#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr int kCudaSkipped = 20;

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_cuda_block_cache model.sdfbin samples.csv "
         "[--repeat N] [--warmup N] [--mode phi-only|phi-normal] "
         "[--threshold value] [--selection-band value] [--extra-margin value] "
         "[--no-radius] [--no-neighbors] [--cache-max-blocks N] "
         "[--cache-max-mb value] [--compare-cpu-active] [--compare-direct] "
         "[--compare-global] [--csv benchmark.csv] [--report benchmark.md] "
         "[--json benchmark.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::string boolText(bool value) {
  return value ? "true" : "false";
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

std::string csv(
    const adasdf::CudaActiveBlockBenchmarkResult& result,
    const adasdf::CudaActiveBlockBenchmarkOptions& options,
    const std::string& mode) {
  std::ostringstream out;
  out << "sample_count,repeat,warmup,mode,cuda_available,"
         "active_block_count,gpu_memory_bytes,cuda_total_avg_ms,"
         "cuda_kernel_avg_ms,cuda_upload_avg_ms,cuda_download_avg_ms,"
         "cuda_ns_per_sample,cpu_active_avg_ms,direct_sparse_avg_ms,"
         "global_expanded_avg_ms,status,error_message\n";
  out << result.sample_count << ","
      << options.repeat << ","
      << options.warmup << ","
      << mode << ","
      << boolText(result.cuda_available) << ","
      << result.active_block_count << ","
      << result.gpu_memory_bytes << ","
      << result.cuda_total_avg_ms << ","
      << result.cuda_kernel_avg_ms << ","
      << result.cuda_upload_avg_ms << ","
      << result.cuda_download_avg_ms << ","
      << result.cuda_ns_per_sample << ","
      << result.cpu_active_avg_ms << ","
      << result.direct_sparse_avg_ms << ","
      << result.global_expanded_avg_ms << ","
      << (result.success ? "ok" : "skipped") << ","
      << "\"" << result.error_message << "\"\n";
  return out.str();
}

std::string markdown(
    const adasdf::CudaActiveBlockBenchmarkResult& result,
    const std::string& mode) {
  std::ostringstream out;
  out << "# CUDA Active Block Cache Benchmark\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- CUDA available: " << boolText(result.cuda_available) << "\n";
  out << "- Mode: " << mode << "\n";
  out << "- Sample count: " << result.sample_count << "\n";
  out << "- Active blocks: " << result.active_block_count << "\n";
  out << "- GPU memory bytes: " << result.gpu_memory_bytes << "\n";
  out << "- CUDA total avg ms: " << result.cuda_total_avg_ms << "\n";
  out << "- CUDA kernel avg ms: " << result.cuda_kernel_avg_ms << "\n";
  out << "- CUDA upload avg ms: " << result.cuda_upload_avg_ms << "\n";
  out << "- CUDA download avg ms: " << result.cuda_download_avg_ms << "\n";
  out << "- CUDA ns per sample: " << result.cuda_ns_per_sample << "\n";
  out << "- CPU active avg ms: " << result.cpu_active_avg_ms << "\n";
  out << "- Direct sparse avg ms: " << result.direct_sparse_avg_ms << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "\nThis benchmark keeps kernel-only and total-time fields visible. "
         "The v1.11 CUDA path uploads CPU-expanded active blocks, then queries "
         "those local dense blocks on GPU.\n";
  return out.str();
}

std::string json(
    const adasdf::CudaActiveBlockBenchmarkResult& result,
    const std::string& mode) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"cuda_available\": " << boolText(result.cuda_available) << ",\n";
  out << "  \"mode\": \"" << mode << "\",\n";
  out << "  \"sample_count\": " << result.sample_count << ",\n";
  out << "  \"active_block_count\": " << result.active_block_count << ",\n";
  out << "  \"gpu_memory_bytes\": " << result.gpu_memory_bytes << ",\n";
  out << "  \"cuda_total_avg_ms\": " << result.cuda_total_avg_ms << ",\n";
  out << "  \"cuda_kernel_avg_ms\": " << result.cuda_kernel_avg_ms << ",\n";
  out << "  \"cuda_upload_avg_ms\": " << result.cuda_upload_avg_ms << ",\n";
  out << "  \"cuda_download_avg_ms\": " << result.cuda_download_avg_ms << ",\n";
  out << "  \"cpu_active_avg_ms\": " << result.cpu_active_avg_ms << ",\n";
  out << "  \"direct_sparse_avg_ms\": " << result.direct_sparse_avg_ms << "\n";
  out << "}\n";
  return out.str();
}

void printResult(
    const adasdf::CudaActiveBlockBenchmarkResult& result,
    const std::string& mode) {
  std::cout << "AdaSDF-CL CUDA active block cache benchmark\n";
  std::cout << "CUDA available: " << boolText(result.cuda_available) << "\n";
  std::cout << "CUDA block cache benchmark mode: " << mode << "\n";
  std::cout << "Sample count: " << result.sample_count << "\n";
  std::cout << "Active blocks: " << result.active_block_count << "\n";
  std::cout << "GPU memory bytes: " << result.gpu_memory_bytes << "\n";
  std::cout << "CUDA total avg ms: " << result.cuda_total_avg_ms << "\n";
  std::cout << "CUDA kernel avg ms: " << result.cuda_kernel_avg_ms << "\n";
  std::cout << "CUDA upload avg ms: " << result.cuda_upload_avg_ms << "\n";
  std::cout << "CUDA download avg ms: " << result.cuda_download_avg_ms << "\n";
  std::cout << "Average ns per sample: " << result.cuda_ns_per_sample << "\n";
  std::cout << "CPU active avg ms: " << result.cpu_active_avg_ms << "\n";
  std::cout << "Direct sparse avg ms: " << result.direct_sparse_avg_ms << "\n";
  if (!result.error_message.empty()) {
    std::cout << "Error: " << result.error_message << "\n";
  }
  std::cout << "Status: " << (result.success ? "ok" : "skipped") << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 3) {
      usage();
      return 1;
    }

    const std::filesystem::path model_path = argv[1];
    const std::filesystem::path samples_path = argv[2];
    adasdf::CudaActiveBlockBenchmarkOptions options;
    std::string mode = "phi-only";
    std::filesystem::path csv_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--repeat" && hasValue(i, argc)) {
        options.repeat = std::stoi(argv[++i]);
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        options.warmup = std::stoi(argv[++i]);
      } else if (arg == "--mode" && hasValue(i, argc)) {
        mode = argv[++i];
        if (mode == "phi-only") {
          options.with_normal = false;
        } else if (mode == "phi-normal" || mode == "phi+normal") {
          options.with_normal = true;
        } else {
          std::cerr << "Unsupported mode: " << mode << "\n";
          return 1;
        }
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--selection-band" && hasValue(i, argc)) {
        options.selection_band = std::stod(argv[++i]);
      } else if (arg == "--extra-margin" && hasValue(i, argc)) {
        options.extra_margin = std::stod(argv[++i]);
      } else if (arg == "--no-radius") {
        options.query_options.use_sample_radius = false;
      } else if (arg == "--no-neighbors") {
        options.query_options.include_neighbors = false;
      } else if ((arg == "--cache-max-blocks" ||
                  arg == "--max-cache-blocks") &&
                 hasValue(i, argc)) {
        options.query_options.cache_options.cpu_max_blocks =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if ((arg == "--cache-max-mb" || arg == "--cache-mb") &&
                 hasValue(i, argc)) {
        const std::size_t bytes = static_cast<std::size_t>(
            std::stod(argv[++i]) * 1024.0 * 1024.0);
        options.query_options.cache_options.cpu_max_memory_bytes = bytes;
        options.query_options.cache_options.gpu_max_memory_bytes = bytes;
      } else if (arg == "--compare-cpu-active") {
        options.compare_cpu_active_block = true;
      } else if (arg == "--no-compare-cpu-active") {
        options.compare_cpu_active_block = false;
      } else if (arg == "--compare-direct") {
        options.compare_direct_sparse = true;
      } else if (arg == "--no-compare-direct") {
        options.compare_direct_sparse = false;
      } else if (arg == "--compare-global") {
        options.compare_global_expanded = true;
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr
          << "adasdf_benchmark_cuda_block_cache: failed to load queryable model\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr
          << "adasdf_benchmark_cuda_block_cache: failed to read samples: "
          << samples.error_message << "\n";
      return 1;
    }

    const adasdf::CudaActiveBlockBenchmarkResult result =
        adasdf::CudaActiveBlockBenchmark::run(
            *model, samples.sample_set, options);
    printResult(result, mode);

    if (!csv_path.empty() && !writeText(csv_path, csv(result, options, mode))) {
      std::cerr << "adasdf_benchmark_cuda_block_cache: failed to write CSV\n";
      return 2;
    }
    if (!report_path.empty() &&
        !writeText(report_path, markdown(result, mode))) {
      std::cerr << "adasdf_benchmark_cuda_block_cache: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() && !writeText(json_path, json(result, mode))) {
      std::cerr << "adasdf_benchmark_cuda_block_cache: failed to write JSON\n";
      return 2;
    }
    if (!result.cuda_available) {
      return kCudaSkipped;
    }
    return result.success ? 0 : 2;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_cuda_block_cache failed: " << exc.what()
              << "\n";
    return 2;
  }
}
