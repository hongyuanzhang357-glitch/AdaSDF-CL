#include "adasdf/io/DemoAdaptiveSDFBin.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string readRest(std::istringstream& tokens) {
  std::string rest;
  std::getline(tokens, rest);
  while (!rest.empty() && rest.front() == ' ') {
    rest.erase(rest.begin());
  }
  return rest;
}

void requireFinite(const Vector3& value, const char* label) {
  if (!value.allFinite()) {
    throw std::runtime_error(
        std::string("Demo adaptive .sdfbin ") + label + " must be finite.");
  }
}

void requirePositive(const Vector3& value, const char* label) {
  requireFinite(value, label);
  if (!(value.x > 0.0) || !(value.y > 0.0) || !(value.z > 0.0)) {
    throw std::runtime_error(
        std::string("Demo adaptive .sdfbin ") + label + " must be positive.");
  }
}

}  // namespace

const char* DemoAdaptiveSDFBin::magic() {
  return DemoAdaptiveSDFModel::formatMagic();
}

bool DemoAdaptiveSDFBin::canRead(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string first_line;
  std::getline(input, first_line);
  return trimCarriageReturn(first_line) == magic();
}

std::shared_ptr<DemoAdaptiveSDFModel> DemoAdaptiveSDFBin::read(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error(
        "Could not open demo adaptive .sdfbin: " + path.string());
  }

  std::string line;
  std::getline(input, line);
  if (trimCarriageReturn(line) != magic()) {
    throw std::runtime_error("File is not an ADASDF demo adaptive .sdfbin.");
  }

  DemoAdaptiveSDFDescription description;
  std::vector<DemoOctreeNodeInfo> nodes;
  std::vector<DemoBlockInfo> blocks;
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
      tokens >> description.shape_type;
      description.shape_type = lower(description.shape_type);
      saw_shape = true;
    } else if (key == "center") {
      tokens >> description.center.x >> description.center.y >> description.center.z;
      saw_center = true;
    } else if (key == "half_extent") {
      tokens >> description.half_extent.x >> description.half_extent.y >>
          description.half_extent.z;
      saw_half_extent = true;
    } else if (key == "unit") {
      tokens >> description.unit;
    } else if (key == "target_near_surface_error") {
      tokens >> description.target_near_surface_error;
    } else if (key == "memory_limit_mb") {
      tokens >> description.memory_limit_mb;
    } else if (key == "block_expand_limit_mb") {
      tokens >> description.block_expand_limit_mb;
    } else if (key == "octree_nodes") {
      int ignored_count = 0;
      tokens >> ignored_count;
    } else if (key == "octree_node") {
      DemoOctreeNodeInfo node;
      int near_surface = 0;
      std::string min_token;
      std::string max_token;
      std::string near_token;
      tokens >> node.level >> min_token >> node.min_corner.x >>
          node.min_corner.y >> node.min_corner.z >> max_token >>
          node.max_corner.x >> node.max_corner.y >> node.max_corner.z >>
          near_token >> near_surface;
      if (min_token != "min" || max_token != "max" ||
          near_token != "near_surface") {
        throw std::runtime_error("Malformed octree_node line in demo adaptive .sdfbin.");
      }
      node.near_surface = near_surface != 0;
      nodes.push_back(node);
    } else if (key == "blocks") {
      int ignored_count = 0;
      tokens >> ignored_count;
    } else if (key == "block") {
      DemoBlockInfo block;
      std::string min_token;
      std::string max_token;
      std::string res_token;
      std::string rank_token;
      std::string error_token;
      std::string memory_token;
      tokens >> block.block_id >> min_token >> block.min_corner.x >>
          block.min_corner.y >> block.min_corner.z >> max_token >>
          block.max_corner.x >> block.max_corner.y >> block.max_corner.z >>
          res_token >> block.resolution >> rank_token >> block.rank >>
          error_token >> block.estimated_error >> memory_token >>
          block.estimated_memory_kb;
      if (min_token != "min" || max_token != "max" ||
          res_token != "resolution" || rank_token != "rank" ||
          error_token != "estimated_error" ||
          memory_token != "estimated_memory_kb") {
        throw std::runtime_error("Malformed block line in demo adaptive .sdfbin.");
      }
      blocks.push_back(block);
    } else if (key == "surrogate") {
      tokens >> description.surrogate_id;
    } else if (key == "warning") {
      description.warning = readRest(tokens);
    } else if (key == "max_octree_level") {
      tokens >> description.build_options.max_octree_level;
    } else if (key == "block_resolution") {
      tokens >> description.build_options.base_block_cells;
    } else if (key == "rank_budget") {
      tokens >> description.build_options.max_rank;
    } else {
      throw std::runtime_error(
          "Unknown key in demo adaptive .sdfbin: " + key);
    }
    if (!tokens && key != "warning") {
      throw std::runtime_error(
          "Malformed line in demo adaptive .sdfbin: " + line);
    }
  }

  if (!saw_shape || description.shape_type != "box") {
    throw std::runtime_error(
        "Demo adaptive .sdfbin currently requires 'shape box'.");
  }
  if (!saw_center) {
    throw std::runtime_error("Demo adaptive .sdfbin missing center.");
  }
  if (!saw_half_extent) {
    throw std::runtime_error("Demo adaptive .sdfbin missing half_extent.");
  }
  requireFinite(description.center, "center");
  requirePositive(description.half_extent, "half_extent");
  if (nodes.empty() || blocks.empty()) {
    throw std::runtime_error(
        "Demo adaptive .sdfbin missing octree or block metadata.");
  }

  description.build_options.near_surface_error =
      description.target_near_surface_error;
  description.build_options.tau_near_abs = description.target_near_surface_error;
  description.build_options.memory_limit_mb = description.memory_limit_mb;
  description.build_options.block_memory_limit_mb =
      description.block_expand_limit_mb;
  description.build_options.max_memory_mb =
      static_cast<std::size_t>(description.memory_limit_mb);
  description.build_options.block_expand_limit_mb =
      static_cast<std::size_t>(description.block_expand_limit_mb);
  description.build_options.use_surrogate_recommendation = true;

  auto model = DemoAdaptiveSDFModel::create(
      description,
      std::move(nodes),
      std::move(blocks));
  model->setSourcePath(path.string());
  return model;
}

void DemoAdaptiveSDFBin::write(
    const std::filesystem::path& path,
    const DemoAdaptiveSDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("DemoAdaptiveSDFBin::write received an empty path.");
  }
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    throw std::runtime_error(
        "Could not write demo adaptive .sdfbin: " + path.string());
  }

  const auto& description = model.description();
  output << magic() << "\n";
  output << "shape " << description.shape_type << "\n";
  output << "center " << description.center << "\n";
  output << "half_extent " << description.half_extent << "\n";
  output << "unit " << description.unit << "\n";
  output << "target_near_surface_error "
         << description.target_near_surface_error << "\n";
  output << "memory_limit_mb " << description.memory_limit_mb << "\n";
  output << "block_expand_limit_mb "
         << description.block_expand_limit_mb << "\n";
  output << "max_octree_level "
         << description.build_options.max_octree_level << "\n";
  output << "block_resolution "
         << description.build_options.base_block_cells << "\n";
  output << "rank_budget " << description.build_options.max_rank << "\n";
  output << "octree_nodes " << model.octreeNodes().size() << "\n";
  for (const DemoOctreeNodeInfo& node : model.octreeNodes()) {
    output << "octree_node " << node.level << " min " << node.min_corner
           << " max " << node.max_corner << " near_surface "
           << (node.near_surface ? 1 : 0) << "\n";
  }
  output << "blocks " << model.blocks().size() << "\n";
  for (const DemoBlockInfo& block : model.blocks()) {
    output << "block " << block.block_id << " min " << block.min_corner
           << " max " << block.max_corner << " resolution "
           << block.resolution << " rank " << block.rank
           << " estimated_error " << block.estimated_error
           << " estimated_memory_kb " << block.estimated_memory_kb << "\n";
  }
  output << "surrogate " << description.surrogate_id << "\n";
  output << "warning " << description.warning << "\n";
}

}  // namespace adasdf
