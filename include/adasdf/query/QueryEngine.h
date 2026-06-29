#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adasdf/query/BatchQuery.h"
#include "adasdf/query/ExpandedSDF.h"
#include "adasdf/query/QueryModeConfig.h"
#include "adasdf/query/SDFExpander.h"

namespace adasdf {

struct QueryEngineStats {
  std::string backend;
  std::string expansion_mode;
  std::size_t expanded_memory_bytes = 0;
  std::size_t gpu_resident_memory_bytes = 0;
  std::size_t num_queries = 0;
  std::size_t fallback_count = 0;
  double setup_ms = 0.0;
  double query_kernel_ms = 0.0;
  double query_total_ms = 0.0;
  double total_ms = 0.0;
};

class CudaResidentExpandedSDF;

class QueryEngine {
 public:
  QueryEngine(std::shared_ptr<SDFModel> model, QueryModeConfig config);
  QueryEngine(
      std::shared_ptr<SDFModel> model,
      QueryModeConfig config,
      ExpansionOptions expansion_options);
  ~QueryEngine();

  QueryEngine(const QueryEngine&) = delete;
  QueryEngine& operator=(const QueryEngine&) = delete;
  QueryEngine(QueryEngine&&) noexcept;
  QueryEngine& operator=(QueryEngine&&) noexcept;

  bool prepare();
  double sampleDistance(const Vector3& p);
  Vector3 sampleGradient(const Vector3& p);

  BatchQueryOutput queryBatch(const std::vector<Vector3>& points);

  const QueryEngineStats& stats() const {
    return stats_;
  }

  std::string description() const;

 private:
  bool prepareCPUExpanded();
  bool prepareCUDAExpanded();
  void ensurePrepared();

  std::shared_ptr<SDFModel> model_;
  QueryModeConfig config_;
  ExpansionOptions expansion_options_;
  ExpandedSDF expanded_;
  std::unique_ptr<CudaResidentExpandedSDF> resident_;
  QueryEngineStats stats_;
  QueryExecutionMode execution_mode_ = QueryExecutionMode::DirectCPU;
  bool prepared_ = false;
  bool cuda_active_ = false;
};

}  // namespace adasdf
