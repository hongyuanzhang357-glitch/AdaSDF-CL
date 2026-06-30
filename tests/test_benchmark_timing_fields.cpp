#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ADASDF_CL_BENCHMARK_BATCH_QUERY
#define ADASDF_CL_BENCHMARK_BATCH_QUERY ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

std::string readFile(const std::string& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  return tool;
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
  return "\"" + tool + "\"";
#endif
}

}  // namespace

int main() {
  const std::string tool = ADASDF_CL_BENCHMARK_BATCH_QUERY;
  if (tool.empty()) {
    std::cout << "SKIPPED: benchmark executable was not built\n";
    return 0;
  }

  const std::string out = std::string(ADASDF_CL_TEST_TEMP_DIR) +
                          "/benchmark_timing_fields.csv";
  const std::string command =
      executableCommand(tool) +
      " --points 64 --query-backend cpu --expansion none "
      "--out \"" + out + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    std::cerr << "benchmark command failed\n";
    return 1;
  }

  const std::string csv = readFile(out);
  const std::vector<std::string> required = {
      "setup_ms",
      "expand_ms",
      "upload_sdf_ms",
      "allocation_ms",
      "h2d_points_ms",
      "kernel_ms",
      "sync_ms",
      "d2h_results_ms",
      "postprocess_ms",
      "free_ms",
      "total_ms",
      "max_abs_error",
      "mean_abs_error",
      "rms_error",
      "p95_abs_error",
      "sign_mismatch_count",
      "sign_mismatch_rate",
      "ambiguous_sign_count",
      "ambiguous_sign_rate",
      "near_surface_sign_mismatch_rate",
      "fallback_rate",
      "kernel_min_ms",
      "kernel_mean_ms",
      "total_mean_ms",
      "output_mode",
      "phi_only",
      "reuse_resident",
      "workspace_reused",
      "allocation_count",
      "workspace_device_memory_mb",
      "block_lookup_count",
      "block_scan_count",
      "center_block_hit_rate",
      "download_results",
      "correctness_checked"};
  for (const std::string& field : required) {
    if (!contains(csv, field)) {
      std::cerr << "missing benchmark CSV field: " << field << "\n";
      return 1;
    }
  }
  if (!contains(csv, "cpu,none,all,64")) {
    std::cerr << "CPU benchmark row was not generated\n";
    return 1;
  }

  std::cout << "benchmark timing fields are present\n";
  return 0;
}
