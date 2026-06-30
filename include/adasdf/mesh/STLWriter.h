#pragma once

#include <string>

#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct STLWriteOptions {
  bool binary = false;
  std::string solid_name = "adasdf_cleaned_mesh";
};

class STLWriter {
 public:
  static bool write(
      const std::string& path,
      const TriangleMesh& mesh,
      const STLWriteOptions& options = STLWriteOptions{},
      std::string* error_message = nullptr);
};

}  // namespace adasdf
