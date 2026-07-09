#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "adasdf/contract/ErrorCode.h"
#include "adasdf/contract/Warning.h"
#include "adasdf/profile/BuildProfileCounters.h"

namespace adasdf {

struct BuildProfileTimings {
  double load_mesh_time_ms = 0.0;
  double mesh_preprocess_time_ms = 0.0;
  double bvh_build_time_ms = 0.0;
  double distance_query_time_ms = 0.0;
  double sign_query_time_ms = 0.0;
  double adaptive_refinement_time_ms = 0.0;
  double contact_band_marker_time_ms = 0.0;
  double compression_time_ms = 0.0;
  double write_model_time_ms = 0.0;
  double metadata_time_ms = 0.0;
  double cache_lookup_time_ms = 0.0;
  double cache_insert_time_ms = 0.0;
  double deduplication_time_ms = 0.0;
  double marker_cache_time_ms = 0.0;
  double total_time_ms = 0.0;
};

class BuildProfiler {
 public:
  explicit BuildProfiler(bool enabled = false);

  bool enabled() const {
    return enabled_;
  }

  void start();
  void setToolName(std::string tool_name);
  void setInputPath(std::string input_path);
  void setOutputPath(std::string output_path);
  void addWarning(const std::string& code, const std::string& message);
  void finishCompleted();
  void finishTimeout();
  void finishFailed(ErrorCode code, const std::string& message);
  double elapsedMs() const;

  BuildProfileTimings timings;
  BuildProfileCounters counters;
  std::string tool_name;
  std::string input_path;
  std::string output_path;
  std::string status = "completed";
  ErrorCode status_code = ErrorCode::OK;
  bool success = true;
  std::string error_message;
  std::vector<Warning> warnings;

 private:
  bool enabled_ = false;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace adasdf
