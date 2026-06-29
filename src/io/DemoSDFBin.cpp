#include "adasdf/io/DemoSDFBin.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
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

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

void requireFinite(const Vector3& value, const char* label) {
  if (!value.allFinite()) {
    throw std::runtime_error(std::string("Demo .sdfbin ") + label + " must be finite.");
  }
}

void requirePositive(const Vector3& value, const char* label) {
  requireFinite(value, label);
  if (!(value.x > 0.0) || !(value.y > 0.0) || !(value.z > 0.0)) {
    throw std::runtime_error(std::string("Demo .sdfbin ") + label + " must be positive.");
  }
}

}  // namespace

const char* DemoSDFBin::magic() {
  return "ADASDF_DEMO_SDFBIN_V1";
}

bool DemoSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string first_line;
  std::getline(input, first_line);
  return trimCarriageReturn(first_line) == magic();
}

std::shared_ptr<AnalyticSDFModel> DemoSDFBin::read(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Could not open demo .sdfbin: " + path.string());
  }

  std::string line;
  std::getline(input, line);
  if (trimCarriageReturn(line) != magic()) {
    throw std::runtime_error("File is not an ADASDF demo .sdfbin.");
  }

  std::string shape;
  Vector3 center = Vector3::Zero();
  Vector3 half_extent{0.5, 0.5, 0.5};
  std::string unit = "m";
  bool saw_shape = false;
  bool saw_center = false;
  bool saw_half_extent = false;

  while (std::getline(input, line)) {
    line = trimCarriageReturn(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::istringstream tokens(line);
    std::string key;
    tokens >> key;
    key = lower(key);
    if (key == "shape") {
      tokens >> shape;
      shape = lower(shape);
      saw_shape = true;
    } else if (key == "center") {
      tokens >> center.x >> center.y >> center.z;
      saw_center = true;
    } else if (key == "half_extent") {
      tokens >> half_extent.x >> half_extent.y >> half_extent.z;
      saw_half_extent = true;
    } else if (key == "unit") {
      tokens >> unit;
    } else {
      throw std::runtime_error("Unknown key in demo .sdfbin: " + key);
    }
    if (!tokens) {
      throw std::runtime_error("Malformed line in demo .sdfbin: " + line);
    }
  }

  if (!saw_shape || shape != "box") {
    throw std::runtime_error("Demo .sdfbin currently requires 'shape box'.");
  }
  if (!saw_center) {
    throw std::runtime_error("Demo .sdfbin missing center.");
  }
  if (!saw_half_extent) {
    throw std::runtime_error("Demo .sdfbin missing half_extent.");
  }
  requireFinite(center, "center");
  requirePositive(half_extent, "half_extent");

  auto model = AnalyticSDFModel::createBox(center, half_extent, unit);
  model->setSourcePath(path.string());
  return model;
}

void DemoSDFBin::write(
    const std::filesystem::path& path,
    const AnalyticSDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("DemoSDFBin::write received an empty path.");
  }
  if (model.shapeType() != AnalyticShapeType::Box) {
    throw std::runtime_error("DemoSDFBin::write currently supports box models only.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Could not write demo .sdfbin: " + path.string());
  }

  output << magic() << "\n";
  output << "shape box\n";
  output << "center " << model.center() << "\n";
  output << "half_extent " << model.halfExtent() << "\n";
  output << "unit " << model.unit() << "\n";
}

}  // namespace adasdf
