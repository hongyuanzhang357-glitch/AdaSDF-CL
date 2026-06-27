#pragma once

#include <string>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class SDFBinWriter {
 public:
  static void write(const std::string& path, const SDFModel& model);
};

}  // namespace adasdf
