#include "adasdf/query/SDFExpander.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace adasdf {
namespace {

void requireModel(const SDFModel& model) {
  if (!model.isValid() || !model.queryBackendAvailable()) {
    throw std::runtime_error(
        "SDFExpander requires a valid SDFModel with query support.");
  }
  if (!model.boundingBox().valid || !model.boundingBox().min.allFinite() ||
      !model.boundingBox().max.allFinite()) {
    throw std::runtime_error("SDFExpander requires a finite model bounding box.");
  }
}

int checkedResolution(int resolution, const char* label) {
  if (resolution < 2) {
    throw std::runtime_error(std::string(label) + " must be at least 2.");
  }
  return resolution;
}

Vector3 paddedMin(const AABB& bounds, double padding) {
  return {bounds.min.x - padding, bounds.min.y - padding, bounds.min.z - padding};
}

Vector3 paddedMax(const AABB& bounds, double padding) {
  return {bounds.max.x + padding, bounds.max.y + padding, bounds.max.z + padding};
}

Vector3 samplePoint(
    const Vector3& min_corner,
    const Vector3& max_corner,
    int i,
    int j,
    int k,
    int nx,
    int ny,
    int nz) {
  const auto axis = [](double lo, double hi, int index, int n) {
    return lo + (hi - lo) * static_cast<double>(index) /
                    static_cast<double>(std::max(1, n - 1));
  };
  return {
      axis(min_corner.x, max_corner.x, i, nx),
      axis(min_corner.y, max_corner.y, j, ny),
      axis(min_corner.z, max_corner.z, k, nz)};
}

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

ExpandedBlock makeDenseBlock(
    const SDFModel& model,
    int block_id,
    const Vector3& min_corner,
    const Vector3& max_corner,
    int nx,
    int ny,
    int nz) {
  if (!(min_corner.x < max_corner.x) || !(min_corner.y < max_corner.y) ||
      !(min_corner.z < max_corner.z)) {
    throw std::runtime_error("Cannot expand an empty SDF block.");
  }
  ExpandedBlock block;
  block.block_id = block_id;
  block.min_corner = min_corner;
  block.max_corner = max_corner;
  block.resolution_x = nx;
  block.resolution_y = ny;
  block.resolution_z = nz;
  block.values.resize(
      static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
      static_cast<std::size_t>(nz));

  for (int k = 0; k < nz; ++k) {
    for (int j = 0; j < ny; ++j) {
      for (int i = 0; i < nx; ++i) {
        const Vector3 p = samplePoint(min_corner, max_corner, i, j, k, nx, ny, nz);
        block.values[valueIndex(i, j, k, nx, ny)] = model.sampleDistance(p);
      }
    }
  }
  return block;
}

std::vector<SDFBlockMetadata> selectedMetadata(
    const SDFModel& model,
    const BlockSelection& selection) {
  std::vector<SDFBlockMetadata> metadata = model.blockMetadata();
  if (metadata.empty()) {
    SDFBlockMetadata synthetic;
    synthetic.block_id = 0;
    synthetic.local_min = model.boundingBox().min;
    synthetic.local_max = model.boundingBox().max;
    synthetic.origin = synthetic.local_min;
    metadata.push_back(synthetic);
  }

  if (selection.use_all_blocks) {
    return metadata;
  }

  std::unordered_set<int> wanted(
      selection.block_ids.begin(),
      selection.block_ids.end());
  std::vector<SDFBlockMetadata> selected;
  for (const SDFBlockMetadata& block : metadata) {
    if (wanted.erase(static_cast<int>(block.block_id)) > 0) {
      selected.push_back(block);
    }
  }
  if (!wanted.empty()) {
    std::ostringstream out;
    out << "Requested block id does not exist:";
    for (const int id : wanted) {
      out << " " << id;
    }
    throw std::runtime_error(out.str());
  }
  return selected;
}

}  // namespace

ExpandedSDF SDFExpander::expand(
    const SDFModel& model,
    const ExpansionOptions& options) {
  ExpansionOptions normalized = options;
  if (normalized.expansion == QueryExpansionMode::Auto) {
    normalized.expansion = QueryExpansionMode::Global;
  }
  switch (normalized.expansion) {
    case QueryExpansionMode::Global: {
      requireModel(model);
      const int resolution =
          checkedResolution(normalized.global_resolution, "global_resolution");
      ExpandedBlock block = makeDenseBlock(
          model,
          0,
          paddedMin(model.boundingBox(), normalized.padding),
          paddedMax(model.boundingBox(), normalized.padding),
          resolution,
          resolution,
          resolution);
      return ExpandedSDF::globalDense(std::move(block));
    }
    case QueryExpansionMode::Block: {
      requireModel(model);
      const int resolution =
          checkedResolution(normalized.block_resolution, "block_resolution");
      const std::vector<SDFBlockMetadata> metadata =
          selectedMetadata(model, normalized.block_selection);
      std::vector<ExpandedBlock> blocks;
      blocks.reserve(metadata.size());
      for (const SDFBlockMetadata& item : metadata) {
        blocks.push_back(makeDenseBlock(
            model,
            static_cast<int>(item.block_id),
            {item.local_min.x - normalized.padding,
             item.local_min.y - normalized.padding,
             item.local_min.z - normalized.padding},
            {item.local_max.x + normalized.padding,
             item.local_max.y + normalized.padding,
             item.local_max.z + normalized.padding},
            resolution,
            resolution,
            resolution));
      }
      return ExpandedSDF::blockDense(std::move(blocks));
    }
    case QueryExpansionMode::None:
      throw std::runtime_error("SDFExpander cannot expand QueryExpansionMode::None.");
    case QueryExpansionMode::Auto:
      break;
  }
  throw std::runtime_error("Unsupported expansion mode.");
}

ExpandedSDF SDFExpander::expandGlobal(
    const SDFModel& model,
    int resolution) {
  ExpansionOptions options;
  options.expansion = QueryExpansionMode::Global;
  options.global_resolution = resolution;
  return expand(model, options);
}

ExpandedSDF SDFExpander::expandBlocks(
    const SDFModel& model,
    const BlockSelection& blocks,
    int block_resolution) {
  ExpansionOptions options;
  options.expansion = QueryExpansionMode::Block;
  options.block_selection = blocks;
  options.block_resolution = block_resolution;
  return expand(model, options);
}

}  // namespace adasdf
