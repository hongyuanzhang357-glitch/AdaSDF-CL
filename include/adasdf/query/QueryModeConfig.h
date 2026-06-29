#pragma once

#include <string>
#include <vector>

namespace adasdf {

enum class QueryBackend {
  CPU,
  CUDA,
  Auto
};

enum class QueryExpansionMode {
  None,
  Global,
  Block,
  Auto
};

enum class QueryExecutionMode {
  DirectCPU,
  ExpandedCPU,
  ExpandedCUDA
};

struct BlockSelection {
  bool use_all_blocks = true;
  std::vector<int> block_ids;

  static BlockSelection all();
  static BlockSelection selected(std::vector<int> ids);
};

struct QueryModeConfig {
  QueryBackend backend = QueryBackend::CPU;
  QueryExpansionMode expansion = QueryExpansionMode::None;
  BlockSelection block_selection = BlockSelection::all();

  bool keep_expanded_data_resident = true;
  bool allow_fallback_to_cpu = true;
  bool verbose = true;

  static QueryModeConfig cpuDirect();
  static QueryModeConfig cpuGlobalExpanded();
  static QueryModeConfig cpuBlockExpanded(
      BlockSelection blocks = BlockSelection::all());
  static QueryModeConfig cudaGlobalExpanded();
  static QueryModeConfig cudaBlockExpanded(
      BlockSelection blocks = BlockSelection::all());
};

const char* toString(QueryBackend backend);
const char* toString(QueryExpansionMode expansion);
const char* toString(QueryExecutionMode mode);

QueryExecutionMode resolveQueryExecutionMode(const QueryModeConfig& config);
void validateQueryModeConfig(const QueryModeConfig& config);
std::string blockSelectionString(const BlockSelection& selection);

}  // namespace adasdf
