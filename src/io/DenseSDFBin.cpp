#include "adasdf/io/DenseSDFBin.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace adasdf {
namespace {

std::string trimCarriageReturn(std::string value) {
  if (!value.empty() && value.back() == '\r') {
    value.pop_back();
  }
  return value;
}

void requireFinite(const Vector3& value, const char* label) {
  if (!value.allFinite()) {
    throw std::runtime_error(std::string("Dense .sdfbin ") + label + " must be finite.");
  }
}

void requirePositive(const Vector3& value, const char* label) {
  requireFinite(value, label);
  if (!(value.x > 0.0) || !(value.y > 0.0) || !(value.z > 0.0)) {
    throw std::runtime_error(std::string("Dense .sdfbin ") + label + " must be positive.");
  }
}

}  // namespace

const char* DenseSDFBin::magic() {
  return "ADASDF_DENSE_SDFBIN_V1";
}

bool DenseSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string first_line;
  std::getline(input, first_line);
  return trimCarriageReturn(first_line) == magic();
}

std::shared_ptr<DenseSDFModel> DenseSDFBin::read(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Could not open dense .sdfbin: " + path.string());
  }

  std::string line;
  std::getline(input, line);
  if (trimCarriageReturn(line) != magic()) {
    throw std::runtime_error("File is not an ADASDF dense .sdfbin.");
  }

  DenseSDFGrid grid;
  bool saw_origin = false;
  bool saw_spacing = false;
  bool saw_resolution = false;
  bool saw_values = false;

  while (std::getline(input, line)) {
    line = trimCarriageReturn(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream tokens(line);
    std::string key;
    tokens >> key;
    if (key == "unit") {
      tokens >> grid.unit;
    } else if (key == "origin") {
      tokens >> grid.origin.x >> grid.origin.y >> grid.origin.z;
      saw_origin = true;
    } else if (key == "spacing") {
      tokens >> grid.spacing.x >> grid.spacing.y >> grid.spacing.z;
      saw_spacing = true;
    } else if (key == "resolution") {
      tokens >> grid.nx >> grid.ny >> grid.nz;
      saw_resolution = true;
    } else if (key == "signed") {
      int signed_flag = 1;
      tokens >> signed_flag;
      grid.signed_distance = signed_flag != 0;
    } else if (key == "values") {
      saw_values = true;
      break;
    } else {
      throw std::runtime_error("Unknown key in dense .sdfbin: " + key);
    }
    if (!tokens) {
      throw std::runtime_error("Malformed line in dense .sdfbin: " + line);
    }
  }

  if (!saw_origin || !saw_spacing || !saw_resolution || !saw_values) {
    throw std::runtime_error("Dense .sdfbin missing required header fields.");
  }
  requireFinite(grid.origin, "origin");
  requirePositive(grid.spacing, "spacing");
  if (grid.nx < 2 || grid.ny < 2 || grid.nz < 2) {
    throw std::runtime_error("Dense .sdfbin resolution must be at least 2 per axis.");
  }

  const std::size_t expected =
      static_cast<std::size_t>(grid.nx) * static_cast<std::size_t>(grid.ny) *
      static_cast<std::size_t>(grid.nz);
  grid.phi.reserve(expected);
  double value = 0.0;
  while (input >> value) {
    grid.phi.push_back(value);
  }
  if (grid.phi.size() != expected) {
    throw std::runtime_error(
        "Dense .sdfbin value count does not match resolution.");
  }

  auto model = std::make_shared<DenseSDFModel>(std::move(grid));
  if (!model->isValid()) {
    throw std::runtime_error("Dense .sdfbin produced an invalid DenseSDFModel.");
  }
  SDFMetadata metadata = model->metadata();
  metadata.source_path = path.string();
  model->setMetadata(metadata);
  return model;
}

void DenseSDFBin::write(
    const std::filesystem::path& path,
    const DenseSDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("DenseSDFBin::write received an empty path.");
  }
  if (!model.isValid() || !DenseSDFModel::isValidGrid(model.grid())) {
    throw std::runtime_error("DenseSDFBin::write requires a valid DenseSDFModel.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Could not write dense .sdfbin: " + path.string());
  }

  const DenseSDFGrid& grid = model.grid();
  output << std::setprecision(std::numeric_limits<double>::max_digits10);
  output << magic() << "\n";
  output << "unit " << grid.unit << "\n";
  output << "origin " << grid.origin << "\n";
  output << "spacing " << grid.spacing << "\n";
  output << "resolution " << grid.nx << " " << grid.ny << " " << grid.nz
         << "\n";
  output << "signed " << (grid.signed_distance ? 1 : 0) << "\n";
  output << "values\n";
  for (const double phi : grid.phi) {
    output << phi << "\n";
  }
}

}  // namespace adasdf
