#pragma once

#include <string>
#include <vector>

#include "adasdf/sparse/CollisionSampleSet.h"

namespace adasdf {

struct CollisionSampleSetReadResult {
  bool success = false;
  std::string error_message;
  CollisionSampleSet sample_set;
  std::vector<std::string> warnings;
};

class CollisionSampleSetIO {
 public:
  static CollisionSampleSetReadResult readCSV(const std::string& path);

  static bool writeCSV(
      const std::string& path,
      const CollisionSampleSet& sample_set,
      std::string* error_message = nullptr);
};

}  // namespace adasdf
