#pragma once

#include <string>
#include <vector>

namespace adasdf {

struct SurrogateEstimatorOptions {
  double memory_safety_factor = 1.35;
  double error_safety_factor = 1.50;
  double compression_ratio_safety_factor = 0.80;
  int min_max_level = 3;
  int max_max_level = 7;
  int min_block_resolution = 6;
  int max_block_resolution = 16;
  int min_rank = 2;
  int max_rank = 12;
};

struct SurrogateProfileLoadResult {
  SurrogateEstimatorOptions options;
  bool loaded = false;
  std::vector<std::string> warnings;
};

class SurrogateProfile {
 public:
  static const char* builtInProfileId();
  static const char* statusWarning();

  static SurrogateProfileLoadResult defaults();
  static SurrogateProfileLoadResult load(
      const std::string& path,
      const SurrogateEstimatorOptions& fallback =
          SurrogateEstimatorOptions{});
};

}  // namespace adasdf
