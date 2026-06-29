#include "adasdf/query/QueryModeConfig.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace adasdf {

BlockSelection BlockSelection::all() {
  return {};
}

BlockSelection BlockSelection::selected(std::vector<int> ids) {
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  BlockSelection selection;
  selection.use_all_blocks = false;
  selection.block_ids = std::move(ids);
  return selection;
}

QueryModeConfig QueryModeConfig::cpuDirect() {
  QueryModeConfig config;
  config.backend = QueryBackend::CPU;
  config.expansion = QueryExpansionMode::None;
  return config;
}

QueryModeConfig QueryModeConfig::cpuGlobalExpanded() {
  QueryModeConfig config;
  config.backend = QueryBackend::CPU;
  config.expansion = QueryExpansionMode::Global;
  return config;
}

QueryModeConfig QueryModeConfig::cpuBlockExpanded(BlockSelection blocks) {
  QueryModeConfig config;
  config.backend = QueryBackend::CPU;
  config.expansion = QueryExpansionMode::Block;
  config.block_selection = std::move(blocks);
  return config;
}

QueryModeConfig QueryModeConfig::cudaGlobalExpanded() {
  QueryModeConfig config;
  config.backend = QueryBackend::CUDA;
  config.expansion = QueryExpansionMode::Global;
  return config;
}

QueryModeConfig QueryModeConfig::cudaBlockExpanded(BlockSelection blocks) {
  QueryModeConfig config;
  config.backend = QueryBackend::CUDA;
  config.expansion = QueryExpansionMode::Block;
  config.block_selection = std::move(blocks);
  return config;
}

const char* toString(QueryBackend backend) {
  switch (backend) {
    case QueryBackend::CPU:
      return "cpu";
    case QueryBackend::CUDA:
      return "cuda";
    case QueryBackend::Auto:
      return "auto";
  }
  return "unknown";
}

const char* toString(QueryExpansionMode expansion) {
  switch (expansion) {
    case QueryExpansionMode::None:
      return "none";
    case QueryExpansionMode::Global:
      return "global";
    case QueryExpansionMode::Block:
      return "block";
    case QueryExpansionMode::Auto:
      return "auto";
  }
  return "unknown";
}

const char* toString(QueryExecutionMode mode) {
  switch (mode) {
    case QueryExecutionMode::DirectCPU:
      return "direct_cpu";
    case QueryExecutionMode::ExpandedCPU:
      return "expanded_cpu";
    case QueryExecutionMode::ExpandedCUDA:
      return "expanded_cuda";
  }
  return "unknown";
}

QueryExecutionMode resolveQueryExecutionMode(const QueryModeConfig& config) {
  validateQueryModeConfig(config);
  const QueryBackend backend =
      config.backend == QueryBackend::Auto ? QueryBackend::CPU : config.backend;
  const QueryExpansionMode expansion =
      config.expansion == QueryExpansionMode::Auto
          ? (backend == QueryBackend::CUDA ? QueryExpansionMode::Global
                                           : QueryExpansionMode::None)
          : config.expansion;

  if (backend == QueryBackend::CUDA) {
    return QueryExecutionMode::ExpandedCUDA;
  }
  return expansion == QueryExpansionMode::None
             ? QueryExecutionMode::DirectCPU
             : QueryExecutionMode::ExpandedCPU;
}

void validateQueryModeConfig(const QueryModeConfig& config) {
  const QueryBackend backend =
      config.backend == QueryBackend::Auto ? QueryBackend::CPU : config.backend;
  const QueryExpansionMode expansion =
      config.expansion == QueryExpansionMode::Auto
          ? (backend == QueryBackend::CUDA ? QueryExpansionMode::Global
                                           : QueryExpansionMode::None)
          : config.expansion;

  if (backend == QueryBackend::CUDA && expansion == QueryExpansionMode::None) {
    throw std::runtime_error(
        "CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.");
  }
  if (expansion == QueryExpansionMode::Block &&
      !config.block_selection.use_all_blocks &&
      config.block_selection.block_ids.empty()) {
    throw std::runtime_error(
        "Block expansion requires at least one selected block id or all blocks.");
  }
  if (!config.block_selection.use_all_blocks) {
    for (const int block_id : config.block_selection.block_ids) {
      if (block_id < 0) {
        throw std::runtime_error("Block ids must be non-negative.");
      }
    }
  }
}

std::string blockSelectionString(const BlockSelection& selection) {
  if (selection.use_all_blocks) {
    return "all";
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < selection.block_ids.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << selection.block_ids[i];
  }
  return out.str();
}

}  // namespace adasdf
