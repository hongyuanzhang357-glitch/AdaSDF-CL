#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace {

void usage() {
  std::cout << "Usage: adasdf_info model.sdfbin\n";
}

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

void printAABB(const adasdf::AABB& aabb) {
  if (!aabb.valid) {
    std::cout << "AABB: invalid\n";
    return;
  }
  printVec("AABB min: ", aabb.min);
  printVec("AABB max: ", aabb.max);
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc != 2) {
    usage();
    return 2;
  }

  const std::filesystem::path path = argv[1];
  if (!std::filesystem::exists(path)) {
    std::cerr << "adasdf_info: file does not exist: " << path.string() << "\n";
    return 2;
  }

  try {
    const auto model = SDFBinReader::read(path);
    const SDFMetadata& metadata = model->metadata();
    const double memory_mb =
        static_cast<double>(model->memoryFootprintBytes()) / (1024.0 * 1024.0);

    std::cout << "AdaSDF-CL info\n";
    std::cout << "Library version: " << versionString() << "\n";
    std::cout << "Path: " << path.string() << "\n";
    std::cout << "Model name: " << model->debugName() << "\n";
    std::cout << "Valid: " << yesNo(model->isValid()) << "\n";
    if (!metadata.format_name.empty()) {
      std::cout << "Format: " << metadata.format_name << "\n";
    }
    if (const auto analytic = std::dynamic_pointer_cast<AnalyticSDFModel>(model)) {
      std::cout << "Shape: " << analytic->shapeName() << "\n";
      printVec("Center: ", analytic->center());
      printVec("Half extent: ", analytic->halfExtent());
      std::cout << "Unit: " << analytic->unit() << "\n";
    }
    std::cout << "Format version: " << metadata.format_version << "\n";
    std::cout << "Query backend: "
              << (metadata.query_backend.empty() ? "unavailable" : metadata.query_backend)
              << "\n";
    std::cout << "Query backend available: "
              << yesNo(model->queryBackendAvailable()) << "\n";
    printAABB(model->boundingBox());
    std::cout << "Fine cell count: " << metadata.n_fine_cell << "\n";
    std::cout << "Fine node count: " << metadata.n_fine_node << "\n";
    std::cout << "Fine spacing: " << metadata.h_fine << "\n";
    std::cout << "Max level: " << metadata.max_level << "\n";
    std::cout << "Final blocks: " << metadata.final_block_count << "\n";
    std::cout << "Active blocks: " << metadata.active_block_count << "\n";
    std::cout << "Near-surface blocks: " << metadata.near_surface_block_count << "\n";
    std::cout << "Near-surface error: " << metadata.near_surface_error << "\n";
    std::cout << "Max reconstruction error: "
              << metadata.max_reconstruction_error << "\n";
    std::cout << "Compression ratio: " << metadata.compression_ratio << "\n";
    std::cout << "Memory footprint bytes: " << model->memoryFootprintBytes() << "\n";
    std::cout << "Memory footprint MB: " << std::fixed << std::setprecision(3)
              << memory_mb << "\n";
    return model->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_info failed: " << exc.what() << "\n";
    return 1;
  }
}
