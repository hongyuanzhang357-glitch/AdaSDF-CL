#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool parseUInt64(const std::string& text, std::uint64_t* value) {
  try {
    std::size_t used = 0;
    const auto parsed = std::stoull(text, &used);
    if (used != text.size()) {
      return false;
    }
    *value = static_cast<std::uint64_t>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

bool parseDouble(const std::string& text, double* value) {
  try {
    std::size_t used = 0;
    const double parsed = std::stod(text, &used);
    if (used != text.size() || !std::isfinite(parsed)) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 10) {
    std::cerr
        << "Usage: adasdf_benchmark_sdf_creation INPUT.stl --work-dir DIR "
           "--max-sdf-file-bytes N --max-decoded-block-bytes N "
           "--max-zero-surface-abs-error X [--repeat N] --csv FILE\n";
    return 2;
  }
  const std::filesystem::path input = argv[1];
  std::filesystem::path work_dir;
  std::filesystem::path csv_path;
  adasdf::SDFCreationConstraints constraints;
  int repeat = 1;
  for (int i = 2; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--work-dir" && i + 1 < argc) {
      work_dir = argv[++i];
    } else if (argument == "--csv" && i + 1 < argc) {
      csv_path = argv[++i];
    } else if (argument == "--max-sdf-file-bytes" && i + 1 < argc) {
      if (!parseUInt64(argv[++i], &constraints.max_sdf_file_bytes)) {
        return 2;
      }
    } else if (argument == "--max-decoded-block-bytes" && i + 1 < argc) {
      if (!parseUInt64(argv[++i], &constraints.max_decoded_block_bytes)) {
        return 2;
      }
    } else if (argument == "--max-zero-surface-abs-error" && i + 1 < argc) {
      if (!parseDouble(argv[++i], &constraints.max_zero_surface_abs_error)) {
        return 2;
      }
    } else if (argument == "--repeat" && i + 1 < argc) {
      repeat = std::max(1, std::stoi(argv[++i]));
    } else {
      return 2;
    }
  }
  if (work_dir.empty() || csv_path.empty()) {
    return 2;
  }
  std::filesystem::create_directories(work_dir);
  if (!csv_path.parent_path().empty()) {
    std::filesystem::create_directories(csv_path.parent_path());
  }
  std::ofstream csv(csv_path);
  csv << "repeat,wall_time_ms," << adasdf::sdfCreationCsvHeader() << "\n";
  bool all_feasible = true;
  for (int iteration = 0; iteration < repeat; ++iteration) {
    const std::filesystem::path output =
        work_dir / ("benchmark_" + std::to_string(iteration) + ".sdfbin");
    const auto begin = std::chrono::steady_clock::now();
    const adasdf::SDFCreationResult result =
        adasdf::ConstrainedSDFBuilder::fromSTL(input, output, constraints);
    const double wall_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - begin).count();
    csv << iteration << "," << wall_ms << ","
        << adasdf::toCsvRow(result.report) << "\n";
    all_feasible = all_feasible && result.feasible();
    std::error_code error;
    std::filesystem::remove(output, error);
  }
  return csv && all_feasible ? 0 : 1;
}
