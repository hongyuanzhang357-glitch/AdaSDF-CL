#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr int kCollisionDetected = 10;
constexpr int kCudaSkipped = 20;

void usage() {
  std::cout
      << "Usage: adasdf_cuda_active_block_query model.sdfbin samples.csv "
         "[--threshold value] [--selection-band value] [--extra-margin value] "
         "[--phi-only] [--with-normal] [--early-exit] [--no-radius] "
         "[--no-neighbors] [--no-fallback] [--sort] "
         "[--cache-max-blocks N] [--cache-max-mb value] "
         "[--out results.csv] [--report report.md] [--json report.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::string boolText(bool value) {
  return value ? "true" : "false";
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
      escaped.push_back(ch);
    }
  }
  escaped += "\"";
  return escaped;
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

bool writeQueryCSV(
    const std::filesystem::path& path,
    const adasdf::CudaActiveBlockQueryResult& result) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << std::setprecision(10);
  file << "sample_id,x,y,z,radius,phi,effective_phi,colliding,source,"
          "normal_x,normal_y,normal_z,block_source,object_id,link_id,group_id,"
          "label,status\n";
  for (std::size_t i = 0; i < result.samples.size(); ++i) {
    const adasdf::SparseSDFSampleResult& sample = result.samples[i];
    const std::string source =
        i < result.sample_sources.size() ? result.sample_sources[i] : "";
    file << sample.sample_id << ","
         << sample.position.x << ","
         << sample.position.y << ","
         << sample.position.z << ","
         << sample.radius << ","
         << sample.phi << ","
         << sample.effective_phi << ","
         << boolText(sample.colliding) << ","
         << csvField(source) << ","
         << sample.normal.x << ","
         << sample.normal.y << ","
         << sample.normal.z << ","
         << csvField(source) << ","
         << sample.object_id << ","
         << sample.link_id << ","
         << sample.group_id << ","
         << csvField(sample.label) << ","
         << (result.success ? "ok" : "skipped") << "\n";
  }
  return true;
}

std::string toMarkdown(const adasdf::CudaActiveBlockQueryResult& result) {
  std::ostringstream out;
  out << "# CUDA Active Block Query Report\n\n";
  out << "- Success: " << boolText(result.success) << "\n";
  out << "- CUDA available: " << boolText(result.cuda_available) << "\n";
  out << "- Colliding: " << boolText(result.colliding) << "\n";
  out << "- Sample count: " << result.stats.sample_count << "\n";
  out << "- Queried count: " << result.stats.queried_count << "\n";
  out << "- Result count: " << result.stats.result_count << "\n";
  out << "- Active blocks: " << result.stats.active_block_count << "\n";
  out << "- Expanded blocks: " << result.stats.expanded_block_count << "\n";
  out << "- GPU query count: " << result.stats.gpu_query_count << "\n";
  out << "- CPU fallback queries: "
      << result.stats.cpu_fallback_query_count << "\n";
  out << "- GPU memory bytes: " << result.stats.gpu_memory_bytes << "\n";
  out << "- Selection time ms: " << result.stats.selection_time_ms << "\n";
  out << "- CPU expansion time ms: "
      << result.stats.cpu_expansion_time_ms << "\n";
  out << "- GPU upload time ms: " << result.stats.gpu_upload_time_ms << "\n";
  out << "- Sample upload time ms: "
      << result.stats.sample_upload_time_ms << "\n";
  out << "- Kernel time ms: " << result.stats.kernel_time_ms << "\n";
  out << "- Download time ms: " << result.stats.download_time_ms << "\n";
  out << "- Total time ms: " << result.stats.total_time_ms << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "\nThis v1.11 path selects and expands active blocks on CPU, uploads "
         "expanded dense active blocks to CUDA, and runs local dense-grid "
         "interpolation on GPU. It is not GPU-native compressed SVD "
         "reconstruction.\n";
  return out.str();
}

std::string toJson(const adasdf::CudaActiveBlockQueryResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(result.success) << ",\n";
  out << "  \"cuda_available\": " << boolText(result.cuda_available) << ",\n";
  out << "  \"colliding\": " << boolText(result.colliding) << ",\n";
  out << "  \"sample_count\": " << result.stats.sample_count << ",\n";
  out << "  \"queried_count\": " << result.stats.queried_count << ",\n";
  out << "  \"result_count\": " << result.stats.result_count << ",\n";
  out << "  \"active_block_count\": "
      << result.stats.active_block_count << ",\n";
  out << "  \"expanded_block_count\": "
      << result.stats.expanded_block_count << ",\n";
  out << "  \"gpu_memory_bytes\": " << result.stats.gpu_memory_bytes << ",\n";
  out << "  \"kernel_time_ms\": " << result.stats.kernel_time_ms << ",\n";
  out << "  \"total_time_ms\": " << result.stats.total_time_ms << ",\n";
  out << "  \"error_message\": \"" << result.error_message << "\"\n";
  out << "}\n";
  return out.str();
}

void printResult(const adasdf::CudaActiveBlockQueryResult& result) {
  std::cout << "AdaSDF-CL CUDA active block query\n";
  std::cout << "CUDA available: " << boolText(result.cuda_available) << "\n";
  std::cout << "Sample count: " << result.stats.sample_count << "\n";
  std::cout << "Queried count: " << result.stats.queried_count << "\n";
  std::cout << "Result count: " << result.stats.result_count << "\n";
  std::cout << "Active blocks: " << result.stats.active_block_count << "\n";
  std::cout << "Expanded blocks: " << result.stats.expanded_block_count << "\n";
  std::cout << "GPU query count: " << result.stats.gpu_query_count << "\n";
  std::cout << "Fallback queries: "
            << result.stats.cpu_fallback_query_count << "\n";
  std::cout << "GPU memory bytes: " << result.stats.gpu_memory_bytes << "\n";
  std::cout << "Selection time ms: " << result.stats.selection_time_ms << "\n";
  std::cout << "CPU expansion time ms: "
            << result.stats.cpu_expansion_time_ms << "\n";
  std::cout << "Upload time ms: "
            << result.stats.gpu_upload_time_ms +
                   result.stats.sample_upload_time_ms
            << "\n";
  std::cout << "Block upload time ms: "
            << result.stats.gpu_upload_time_ms << "\n";
  std::cout << "Sample upload time ms: "
            << result.stats.sample_upload_time_ms << "\n";
  std::cout << "Kernel time ms: " << result.stats.kernel_time_ms << "\n";
  std::cout << "Download time ms: " << result.stats.download_time_ms << "\n";
  std::cout << "Total time ms: " << result.stats.total_time_ms << "\n";
  std::cout << "Colliding: " << boolText(result.colliding) << "\n";
  std::cout << "Min effective phi: " << result.stats.min_effective_phi << "\n";
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
    adasdf::CudaActiveBlockQueryOptions options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--selection-band" && hasValue(i, argc)) {
        options.selection_band = std::stod(argv[++i]);
      } else if (arg == "--extra-margin" && hasValue(i, argc)) {
        options.extra_margin = std::stod(argv[++i]);
      } else if (arg == "--phi-only") {
        options.compute_normals = false;
        options.output_mode = adasdf::CudaActiveBlockOutputMode::PhiOnly;
      } else if (arg == "--with-normal") {
        options.compute_normals = true;
        options.output_mode = adasdf::CudaActiveBlockOutputMode::PhiAndNormal;
      } else if (arg == "--early-exit") {
        options.early_exit = true;
      } else if (arg == "--no-radius") {
        options.use_sample_radius = false;
      } else if (arg == "--no-neighbors") {
        options.include_neighbors = false;
      } else if (arg == "--no-fallback") {
        options.fallback_to_cpu_model_query = false;
      } else if (arg == "--sort") {
        options.sort_results_by_effective_phi = true;
      } else if ((arg == "--cache-max-blocks" ||
                  arg == "--max-cache-blocks") &&
                 hasValue(i, argc)) {
        options.cache_options.cpu_max_blocks =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if ((arg == "--cache-max-mb" || arg == "--cache-mb") &&
                 hasValue(i, argc)) {
        const std::size_t bytes = static_cast<std::size_t>(
            std::stod(argv[++i]) * 1024.0 * 1024.0);
        options.cache_options.cpu_max_memory_bytes = bytes;
        options.cache_options.gpu_max_memory_bytes = bytes;
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
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
          << "adasdf_cuda_active_block_query: failed to load queryable model: "
          << model_path.string() << "\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr
          << "adasdf_cuda_active_block_query: failed to read samples: "
          << samples.error_message << "\n";
      return 1;
    }

    const adasdf::CudaActiveBlockQueryResult result =
        adasdf::CudaActiveBlockQuery::query(*model, samples.sample_set, options);

    printResult(result);
    if (!out_path.empty() && !writeQueryCSV(out_path, result)) {
      std::cerr << "adasdf_cuda_active_block_query: failed to write CSV\n";
      return 2;
    }
    if (!report_path.empty() && !writeText(report_path, toMarkdown(result))) {
      std::cerr << "adasdf_cuda_active_block_query: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() && !writeText(json_path, toJson(result))) {
      std::cerr << "adasdf_cuda_active_block_query: failed to write JSON\n";
      return 2;
    }
    if (!result.cuda_available) {
      return kCudaSkipped;
    }
    if (!result.success) {
      return 2;
    }
    return result.colliding ? kCollisionDetected : 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_cuda_active_block_query failed: " << exc.what()
              << "\n";
    return 2;
  }
}
