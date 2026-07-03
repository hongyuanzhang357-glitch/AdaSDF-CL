#include <adasdf/adasdf.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "active_block_test_helpers.h"

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_CUDA_ACTIVE_BLOCK_QUERY
#define ADASDF_CL_CUDA_ACTIVE_BLOCK_QUERY ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos &&
      tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

bool skippedReturnCode(int code) {
  return code == 20 || code == 20 * 256;
}

}  // namespace

int main() {
  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  const adasdf::CudaActiveBlockQueryResult api_result =
      adasdf::CudaActiveBlockQuery::query(
          model, active_block_tests::makeSamples());

  if (adasdf::CudaActiveBlockQuery::isAvailable()) {
    if (!api_result.success) {
      std::cerr << "CUDA available query unexpectedly failed\n";
      return 1;
    }
    std::cout << "SKIP: CUDA is available, unavailable path not exercised\n";
    return 0;
  }

  if (api_result.success || api_result.cuda_available ||
      api_result.error_message.empty()) {
    std::cerr << "CUDA unavailable API contract failed\n";
    return 1;
  }

  const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  const std::string query_tool = ADASDF_CL_CUDA_ACTIVE_BLOCK_QUERY;
  if (build_tool.empty() || query_tool.empty()) {
    std::cout << "SKIP: CUDA active block CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model_path = temp / "cuda_unavailable_compressed.sdfbin";
  const auto stdout_txt = temp / "cuda_unavailable_stdout.txt";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";

  if (std::system((executableCommand(build_tool) + " " + quote(fixture) +
                   " " + quote(model_path) +
                   " --max-level 1 --block-resolution 4 --unsigned "
                   "--fixed-rank 2 > " + quote(temp / "cuda_unavailable_build.txt"))
                      .c_str()) != 0) {
    std::cerr << "failed to build compressed model\n";
    return 1;
  }
  const int rc = std::system(
      (executableCommand(query_tool) + " " + quote(model_path) + " " +
       quote(samples) + " --threshold 1.0 > " + quote(stdout_txt))
          .c_str());
  const std::string stdout_text = readFile(stdout_txt);
  if (!skippedReturnCode(rc) ||
      !contains(stdout_text, "CUDA available: false") ||
      !contains(stdout_text, "Status: skipped")) {
    std::cerr << "CUDA unavailable CLI contract failed with code " << rc
              << "\n";
    return 1;
  }
  std::cout << "CUDA unavailable contract passed\n";
  return 0;
}
