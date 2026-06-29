#include <adasdf/adasdf.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool throwsWith(const adasdf::QueryModeConfig& config, const std::string& text) {
  try {
    adasdf::validateQueryModeConfig(config);
  } catch (const std::runtime_error& error) {
    return std::string(error.what()).find(text) != std::string::npos;
  }
  return false;
}

}  // namespace

int main() {
  adasdf::validateQueryModeConfig(adasdf::QueryModeConfig::cpuDirect());
  adasdf::validateQueryModeConfig(adasdf::QueryModeConfig::cpuGlobalExpanded());
  adasdf::validateQueryModeConfig(adasdf::QueryModeConfig::cpuBlockExpanded());
  adasdf::validateQueryModeConfig(adasdf::QueryModeConfig::cudaGlobalExpanded());
  adasdf::validateQueryModeConfig(adasdf::QueryModeConfig::cudaBlockExpanded());

  if (adasdf::resolveQueryExecutionMode(adasdf::QueryModeConfig::cpuDirect()) !=
      adasdf::QueryExecutionMode::DirectCPU) {
    std::cerr << "CPU direct did not resolve to DirectCPU\n";
    return 1;
  }
  if (adasdf::resolveQueryExecutionMode(adasdf::QueryModeConfig::cpuGlobalExpanded()) !=
      adasdf::QueryExecutionMode::ExpandedCPU) {
    std::cerr << "CPU global did not resolve to ExpandedCPU\n";
    return 1;
  }
  if (adasdf::resolveQueryExecutionMode(adasdf::QueryModeConfig::cudaGlobalExpanded()) !=
      adasdf::QueryExecutionMode::ExpandedCUDA) {
    std::cerr << "CUDA global did not resolve to ExpandedCUDA\n";
    return 1;
  }

  adasdf::QueryModeConfig invalid;
  invalid.backend = adasdf::QueryBackend::CUDA;
  invalid.expansion = adasdf::QueryExpansionMode::None;
  if (!throwsWith(
          invalid,
          "CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.")) {
    std::cerr << "CUDA none mode was not rejected with the expected message\n";
    return 1;
  }

  std::cout << "QueryModeConfig validation passed\n";
  return 0;
}
