#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

struct DenseSDFGrid {
  Vector3 origin;
  Vector3 spacing{1.0, 1.0, 1.0};
  int nx = 0;
  int ny = 0;
  int nz = 0;
  std::vector<double> phi;
  bool signed_distance = true;
  std::string unit = "m";
};

class DenseSDFModel final : public SDFModel {
 public:
  DenseSDFModel();
  explicit DenseSDFModel(DenseSDFGrid grid);

  const DenseSDFGrid& grid() const;
  DenseSDFGrid& grid();

  static bool isValidGrid(const DenseSDFGrid& grid);
  static const char* backendName();

 private:
  std::shared_ptr<DenseSDFGrid> grid_;
};

}  // namespace adasdf
