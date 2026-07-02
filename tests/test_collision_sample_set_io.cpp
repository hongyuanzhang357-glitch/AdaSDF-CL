#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  const std::filesystem::path samples_dir = ADASDF_CL_TEST_SAMPLES_DIR;
  auto read = adasdf::CollisionSampleSetIO::readCSV(
      (samples_dir / "cube_sparse_samples.csv").string());
  if (!read.success || read.sample_set.empty()) {
    std::cerr << "sample CSV read failed: " << read.error_message << "\n";
    return 1;
  }
  if (read.sample_set.samples.front().label != "inside_center") {
    std::cerr << "label parse failed\n";
    return 1;
  }
  auto radius_read = adasdf::CollisionSampleSetIO::readCSV(
      (samples_dir / "cube_sparse_samples_with_radius.csv").string());
  if (!radius_read.success || radius_read.sample_set.samples[1].radius <= 0.0) {
    std::cerr << "radius parse failed\n";
    return 1;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto out = temp / "sample_set_roundtrip.csv";
  std::string error;
  if (!adasdf::CollisionSampleSetIO::writeCSV(
          out.string(), read.sample_set, &error)) {
    std::cerr << "sample CSV write failed: " << error << "\n";
    return 1;
  }
  auto roundtrip = adasdf::CollisionSampleSetIO::readCSV(out.string());
  if (!roundtrip.success || roundtrip.sample_set.size() != read.sample_set.size()) {
    std::cerr << "roundtrip read failed\n";
    return 1;
  }
  auto missing = adasdf::CollisionSampleSetIO::readCSV(
      (samples_dir / "missing.csv").string());
  if (missing.success) {
    std::cerr << "missing file should fail\n";
    return 1;
  }
  std::cout << "collision sample set IO passed\n";
  return 0;
}
