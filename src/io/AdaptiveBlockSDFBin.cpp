#include "adasdf/io/AdaptiveBlockSDFBin.h"

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
    throw std::runtime_error(
        std::string("Adaptive block .sdfbin ") + label + " must be finite.");
  }
}

void requirePositive(const Vector3& value, const char* label) {
  requireFinite(value, label);
  if (!(value.x > 0.0) || !(value.y > 0.0) || !(value.z > 0.0)) {
    throw std::runtime_error(
        std::string("Adaptive block .sdfbin ") + label + " must be positive.");
  }
}

std::size_t valueCount(const AdaptiveSDFBlock& block) {
  return static_cast<std::size_t>(block.nx) *
         static_cast<std::size_t>(block.ny) *
         static_cast<std::size_t>(block.nz);
}

}  // namespace

const char* AdaptiveBlockSDFBin::magic() {
  return "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1";
}

bool AdaptiveBlockSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string first_line;
  std::getline(input, first_line);
  return trimCarriageReturn(first_line) == magic();
}

std::shared_ptr<AdaptiveBlockSDFModel> AdaptiveBlockSDFBin::read(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Could not open adaptive block .sdfbin: " + path.string());
  }

  std::string line;
  std::getline(input, line);
  if (trimCarriageReturn(line) != magic()) {
    throw std::runtime_error("File is not an ADASDF adaptive block .sdfbin.");
  }

  AdaptiveSDFBlockSet block_set;
  std::string unit = "m";
  int expected_blocks = -1;
  int header_resolution = 0;
  bool saw_global_min = false;
  bool saw_global_max = false;

  while (std::getline(input, line)) {
    line = trimCarriageReturn(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream tokens(line);
    std::string key;
    tokens >> key;
    if (key == "unit") {
      tokens >> unit;
    } else if (key == "signed") {
      int signed_flag = 1;
      tokens >> signed_flag;
      block_set.signed_distance = signed_flag != 0;
    } else if (key == "global_min") {
      tokens >> block_set.global_bounds.min.x >> block_set.global_bounds.min.y >>
          block_set.global_bounds.min.z;
      saw_global_min = true;
    } else if (key == "global_max") {
      tokens >> block_set.global_bounds.max.x >> block_set.global_bounds.max.y >>
          block_set.global_bounds.max.z;
      saw_global_max = true;
    } else if (key == "block_count") {
      tokens >> expected_blocks;
    } else if (key == "block_resolution") {
      tokens >> header_resolution;
    } else if (key == "block") {
      AdaptiveSDFBlock block;
      block.signed_distance = block_set.signed_distance;
      bool saw_values = false;
      while (std::getline(input, line)) {
        line = trimCarriageReturn(line);
        if (line.empty() || line[0] == '#') {
          continue;
        }
        std::istringstream block_tokens(line);
        std::string block_key;
        block_tokens >> block_key;
        if (block_key == "id") {
          block_tokens >> block.block_id;
        } else if (block_key == "node_id") {
          block_tokens >> block.octree_node_id;
        } else if (block_key == "level") {
          block_tokens >> block.level;
        } else if (block_key == "near_surface") {
          int flag = 0;
          block_tokens >> flag;
          block.near_surface = flag != 0;
        } else if (block_key == "min") {
          block_tokens >> block.bounds.min.x >> block.bounds.min.y >>
              block.bounds.min.z;
          block.bounds.valid = true;
        } else if (block_key == "max") {
          block_tokens >> block.bounds.max.x >> block.bounds.max.y >>
              block.bounds.max.z;
          block.bounds.valid = true;
        } else if (block_key == "origin") {
          block_tokens >> block.origin.x >> block.origin.y >> block.origin.z;
        } else if (block_key == "spacing") {
          block_tokens >> block.spacing.x >> block.spacing.y >> block.spacing.z;
        } else if (block_key == "resolution") {
          block_tokens >> block.nx >> block.ny >> block.nz;
        } else if (block_key == "values") {
          saw_values = true;
          const std::size_t expected = valueCount(block);
          block.phi.reserve(expected);
          double value = 0.0;
          for (std::size_t i = 0; i < expected; ++i) {
            if (!(input >> value)) {
              throw std::runtime_error(
                  "Adaptive block .sdfbin ended while reading values.");
            }
            block.phi.push_back(value);
          }
          std::string end_token;
          input >> end_token;
          if (end_token != "end_block") {
            throw std::runtime_error(
                "Adaptive block .sdfbin missing end_block marker.");
          }
          std::getline(input, line);
          break;
        } else {
          throw std::runtime_error(
              "Unknown key in adaptive block .sdfbin block: " + block_key);
        }
        if (!block_tokens) {
          throw std::runtime_error(
              "Malformed line in adaptive block .sdfbin: " + line);
        }
      }
      if (!saw_values) {
        throw std::runtime_error("Adaptive block .sdfbin block missing values.");
      }
      requireFinite(block.bounds.min, "block min");
      requireFinite(block.bounds.max, "block max");
      requireFinite(block.origin, "block origin");
      requirePositive(block.spacing, "block spacing");
      if (block.nx < 2 || block.ny < 2 || block.nz < 2) {
        throw std::runtime_error(
            "Adaptive block .sdfbin block resolution must be at least 2.");
      }
      block_set.blocks.push_back(std::move(block));
    } else {
      throw std::runtime_error("Unknown key in adaptive block .sdfbin: " + key);
    }
    if (!tokens) {
      throw std::runtime_error("Malformed line in adaptive block .sdfbin: " + line);
    }
  }

  block_set.global_bounds.valid = saw_global_min && saw_global_max;
  requireFinite(block_set.global_bounds.min, "global_min");
  requireFinite(block_set.global_bounds.max, "global_max");
  if (expected_blocks < 0 ||
      block_set.blocks.size() != static_cast<std::size_t>(expected_blocks)) {
    throw std::runtime_error(
        "Adaptive block .sdfbin block count does not match header.");
  }
  (void)header_resolution;
  (void)unit;

  auto model = std::make_shared<AdaptiveBlockSDFModel>(std::move(block_set));
  if (!model->isValid()) {
    throw std::runtime_error(
        "Adaptive block .sdfbin produced an invalid AdaptiveBlockSDFModel.");
  }
  SDFMetadata metadata = model->metadata();
  metadata.source_path = path.string();
  model->setMetadata(metadata);
  return model;
}

void AdaptiveBlockSDFBin::write(
    const std::filesystem::path& path,
    const AdaptiveBlockSDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("AdaptiveBlockSDFBin::write received an empty path.");
  }
  if (!model.isValid() ||
      !AdaptiveBlockSDFModel::isValidBlockSet(model.blockSet())) {
    throw std::runtime_error(
        "AdaptiveBlockSDFBin::write requires a valid AdaptiveBlockSDFModel.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    throw std::runtime_error(
        "Could not write adaptive block .sdfbin: " + path.string());
  }

  const AdaptiveSDFBlockSet& blocks = model.blockSet();
  int block_resolution = 0;
  if (!blocks.blocks.empty()) {
    block_resolution = blocks.blocks.front().nx;
  }
  output << std::setprecision(std::numeric_limits<double>::max_digits10);
  output << magic() << "\n";
  output << "unit m\n";
  output << "signed " << (blocks.signed_distance ? 1 : 0) << "\n";
  output << "global_min " << blocks.global_bounds.min << "\n";
  output << "global_max " << blocks.global_bounds.max << "\n";
  output << "block_count " << blocks.blocks.size() << "\n";
  output << "block_resolution " << block_resolution << "\n";
  for (const AdaptiveSDFBlock& block : blocks.blocks) {
    output << "block\n";
    output << "id " << block.block_id << "\n";
    output << "node_id " << block.octree_node_id << "\n";
    output << "level " << block.level << "\n";
    output << "near_surface " << (block.near_surface ? 1 : 0) << "\n";
    output << "min " << block.bounds.min << "\n";
    output << "max " << block.bounds.max << "\n";
    output << "origin " << block.origin << "\n";
    output << "spacing " << block.spacing << "\n";
    output << "resolution " << block.nx << " " << block.ny << " "
           << block.nz << "\n";
    output << "values\n";
    for (const double phi : block.phi) {
      output << phi << "\n";
    }
    output << "end_block\n";
  }
}

}  // namespace adasdf
