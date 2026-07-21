#include <adasdf/adasdf.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BENCHMARK_SPARSE_QUERY
#define ADASDF_CL_BENCHMARK_SPARSE_QUERY ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::size_t idx(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::AdaptiveBlockSDFModel makeModel() {
  adasdf::AdaptiveSDFBlockSet set;
  set.global_bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 0;
  block.octree_node_id = 0;
  block.level = 0;
  block.bounds = set.global_bounds;
  block.nx = 4;
  block.ny = 4;
  block.nz = 4;
  block.origin = block.bounds.min;
  block.spacing = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
  block.phi.assign(64, 0.0);
  for (int k = 0; k < 4; ++k) {
    for (int j = 0; j < 4; ++j) {
      for (int i = 0; i < 4; ++i) {
        block.phi[idx(i, j, k, 4, 4)] =
            block.spacing.x * i + block.spacing.y * j + block.spacing.z * k;
      }
    }
  }
  set.blocks.push_back(block);
  return adasdf::AdaptiveBlockSDFModel(set);
}

}  // namespace

int main() {
  if (std::string(ADASDF_CL_BENCHMARK_SPARSE_QUERY).empty()) {
    std::cout << "SKIP: sparse benchmark tool was not built\n";
    return 0;
  }
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "narrowband_brick_query_cli";
  std::filesystem::create_directories(temp);
  const auto model_path = temp / "model.sdfbin";
  const auto samples_path = temp / "samples.csv";
  const auto json_path = temp / "benchmark.json";
  const adasdf::AdaptiveBlockSDFModel model = makeModel();
  adasdf::SDFBinWriter::write(model_path.string(), model);
  {
    std::ofstream samples(samples_path);
    samples << "id,x,y,z,radius,object_id,link_id,group_id,weight,label\n";
    samples << "0,0.25,0.25,0.25,0,0,0,0,1,a\n";
    samples << "1,0.75,0.50,0.25,0,0,0,0,1,b\n";
  }
  const std::string command =
      executableCommand(ADASDF_CL_BENCHMARK_SPARSE_QUERY) + " " +
      quotePath(model_path) + " " + quotePath(samples_path) +
      " --repeat 2 --warmup 1 --mode phi-only --query-backend brick-fast "
      "--report-query-breakdown --json " + quotePath(json_path);
  if (runCommand(command) != 0 || !std::filesystem::exists(json_path)) {
    std::cerr << "brick-fast benchmark CLI failed\n";
    return 1;
  }
  const std::string json = readText(json_path);
  if (!containsText(json, "\"query_backend\": \"brick-fast\"") ||
      !containsText(json, "\"query_breakdown\"") ||
      !containsText(json, "\"max_abs_phi_diff\":0")) {
    std::cerr << "brick-fast benchmark JSON missing expected fields\n";
    return 1;
  }
  std::cout << "narrow-band brick query CLI passed\n";
  return 0;
}
