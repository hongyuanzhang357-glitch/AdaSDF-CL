#include <adasdf/adasdf.h>

#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void usage() {
  std::cerr
      << "Usage: adasdf_create_sdf INPUT.stl OUTPUT.sdfbin "
         "--max-sdf-file-bytes N --max-decoded-block-bytes N "
         "--max-zero-surface-abs-error X [--report report.json] "
         "[--csv report.csv] [--backend-parameters-file parameters.txt]\n";
}

bool parseUInt64(const std::string& text, std::uint64_t* value) {
  try {
    std::size_t used = 0;
    const unsigned long long parsed = std::stoull(text, &used);
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

bool parseInt(const std::string& text, int* value) {
  try {
    std::size_t used = 0;
    const long parsed = std::stol(text, &used);
    if (used != text.size() ||
        parsed < static_cast<long>(std::numeric_limits<int>::min()) ||
        parsed > static_cast<long>(std::numeric_limits<int>::max())) {
      return false;
    }
    *value = static_cast<int>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

std::string trim(const std::string& value) {
  const std::size_t begin = value.find_first_not_of(" \t\r\n");
  const std::size_t end = value.find_last_not_of(" \t\r\n");
  return begin == std::string::npos
      ? std::string{}
      : value.substr(begin, end - begin + 1);
}

bool loadBackendParameters(
    const std::filesystem::path& path,
    adasdf::BackendBuildParameters* parameters,
    std::string* error) {
  std::ifstream input(path);
  if (!input) {
    *error = "failed to open backend parameters file";
    return false;
  }
  std::map<std::string, std::string> values;
  std::string line;
  while (std::getline(input, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      *error = "backend parameter line must be key=value";
      return false;
    }
    const std::string key = trim(line.substr(0, equals));
    const std::string value = trim(line.substr(equals + 1));
    if (key.empty() || value.empty() || !values.emplace(key, value).second) {
      *error = "backend parameter key is empty or duplicated";
      return false;
    }
  }
  const std::set<std::string> expected = {
      "deterministic_seed", "thread_count", "bvh_leaf_size",
      "octree_min_depth", "octree_max_depth",
      "octree_narrow_band_scale", "octree_interpolation_residual_scale",
      "octree_max_overlapping_triangles", "octree_max_normal_complexity",
      "block_min_nodes", "block_target_nodes", "block_max_nodes",
      "block_halo", "block_min_tensor_dimension",
      "block_max_tensor_dimension", "compression_method",
      "compression_abs_tolerance", "compression_max_rank"};
  std::set<std::string> actual;
  for (const auto& item : values) {
    actual.insert(item.first);
  }
  if (actual != expected) {
    *error = "backend parameter file must contain exactly the documented keys";
    return false;
  }
  std::uint64_t seed = 0;
  bool valid = parseUInt64(values["deterministic_seed"], &seed) &&
      seed <= std::numeric_limits<std::uint32_t>::max() &&
      parseInt(values["thread_count"], &parameters->thread_count) &&
      parseInt(values["bvh_leaf_size"], &parameters->bvh_leaf_size) &&
      parseInt(values["octree_min_depth"], &parameters->octree_min_depth) &&
      parseInt(values["octree_max_depth"], &parameters->octree_max_depth) &&
      parseDouble(
          values["octree_narrow_band_scale"],
          &parameters->octree_narrow_band_scale) &&
      parseDouble(
          values["octree_interpolation_residual_scale"],
          &parameters->octree_interpolation_residual_scale) &&
      parseInt(
          values["octree_max_overlapping_triangles"],
          &parameters->octree_max_overlapping_triangles) &&
      parseDouble(
          values["octree_max_normal_complexity"],
          &parameters->octree_max_normal_complexity) &&
      parseInt(values["block_min_nodes"], &parameters->block_min_nodes) &&
      parseInt(values["block_target_nodes"], &parameters->block_target_nodes) &&
      parseInt(values["block_max_nodes"], &parameters->block_max_nodes) &&
      parseInt(values["block_halo"], &parameters->block_halo) &&
      parseInt(
          values["block_min_tensor_dimension"],
          &parameters->block_min_tensor_dimension) &&
      parseInt(
          values["block_max_tensor_dimension"],
          &parameters->block_max_tensor_dimension) &&
      parseDouble(
          values["compression_abs_tolerance"],
          &parameters->compression_abs_tolerance) &&
      parseInt(
          values["compression_max_rank"],
          &parameters->compression_max_rank);
  if (!valid) {
    *error = "backend parameter file contains an invalid numeric value";
    return false;
  }
  parameters->deterministic_seed = static_cast<std::uint32_t>(seed);
  parameters->compression_method = values["compression_method"];
  return adasdf::validateBackendBuildParameters(*parameters, error);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 9) {
    usage();
    return 2;
  }
  const std::filesystem::path input = argv[1];
  const std::filesystem::path output = argv[2];
  std::filesystem::path report_path;
  std::filesystem::path csv_path;
  std::filesystem::path backend_parameters_path;
  adasdf::SDFCreationConstraints constraints;
  for (int i = 3; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--max-sdf-file-bytes" && i + 1 < argc) {
      if (!parseUInt64(argv[++i], &constraints.max_sdf_file_bytes)) {
        std::cerr << "invalid --max-sdf-file-bytes\n";
        return 2;
      }
    } else if (argument == "--max-decoded-block-bytes" && i + 1 < argc) {
      if (!parseUInt64(argv[++i], &constraints.max_decoded_block_bytes)) {
        std::cerr << "invalid --max-decoded-block-bytes\n";
        return 2;
      }
    } else if (argument == "--max-zero-surface-abs-error" && i + 1 < argc) {
      if (!parseDouble(argv[++i], &constraints.max_zero_surface_abs_error)) {
        std::cerr << "invalid --max-zero-surface-abs-error\n";
        return 2;
      }
    } else if (argument == "--report" && i + 1 < argc) {
      report_path = argv[++i];
    } else if (argument == "--csv" && i + 1 < argc) {
      csv_path = argv[++i];
    } else if (argument == "--backend-parameters-file" && i + 1 < argc) {
      backend_parameters_path = argv[++i];
    } else {
      std::cerr << "unknown or incomplete argument: " << argument << "\n";
      usage();
      return 2;
    }
  }

  adasdf::SDFCreationResult result;
  if (backend_parameters_path.empty()) {
    result = adasdf::ConstrainedSDFBuilder::fromSTL(input, output, constraints);
  } else {
    adasdf::BackendBuildParameters parameters;
    std::string error;
    if (!loadBackendParameters(backend_parameters_path, &parameters, &error)) {
      std::cerr << error << "\n";
      return 2;
    }
    result = adasdf::ConstrainedSDFBuilder::fromSTLWithParameters(
        input, output, constraints, parameters);
  }
  const std::string json = adasdf::toJson(result.report) + "\n";
  std::cout << json;
  if (!report_path.empty()) {
    try {
      if (!report_path.parent_path().empty()) {
        std::filesystem::create_directories(report_path.parent_path());
      }
      std::ofstream report(report_path);
      report << json;
      if (!report) {
        std::cerr << "failed to write report\n";
        return 2;
      }
    } catch (const std::exception& exc) {
      std::cerr << exc.what() << "\n";
      return 2;
    }
  }
  if (!csv_path.empty()) {
    try {
      if (!csv_path.parent_path().empty()) {
        std::filesystem::create_directories(csv_path.parent_path());
      }
      std::ofstream csv(csv_path);
      csv << adasdf::sdfCreationCsvHeader() << "\n"
          << adasdf::toCsvRow(result.report) << "\n";
      if (!csv) {
        std::cerr << "failed to write CSV report\n";
        return 2;
      }
    } catch (const std::exception& exc) {
      std::cerr << exc.what() << "\n";
      return 2;
    }
  }
  if (result.report.status == adasdf::SDFCreationStatus::Feasible) {
    return 0;
  }
  if (result.report.status == adasdf::SDFCreationStatus::Infeasible) {
    return 3;
  }
  return 2;
}
