#include "adasdf/geometry/DenseSDFModel.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double finiteOrZero(double value) {
  return std::isfinite(value) ? value : 0.0;
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double sampleGrid(const DenseSDFGrid& grid, const Vector3& point) {
  if (!DenseSDFModel::isValidGrid(grid) || !point.allFinite()) {
    return 0.0;
  }
  const double x = axisCoord(point.x, grid.origin.x, grid.spacing.x, grid.nx);
  const double y = axisCoord(point.y, grid.origin.y, grid.spacing.y, grid.ny);
  const double z = axisCoord(point.z, grid.origin.z, grid.spacing.z, grid.nz);

  const int i0 = static_cast<int>(std::floor(x));
  const int j0 = static_cast<int>(std::floor(y));
  const int k0 = static_cast<int>(std::floor(z));
  const int i1 = std::min(i0 + 1, grid.nx - 1);
  const int j1 = std::min(j0 + 1, grid.ny - 1);
  const int k1 = std::min(k0 + 1, grid.nz - 1);
  const double tx = x - static_cast<double>(i0);
  const double ty = y - static_cast<double>(j0);
  const double tz = z - static_cast<double>(k0);

  const auto value = [&](int i, int j, int k) {
    return finiteOrZero(grid.phi[valueIndex(i, j, k, grid.nx, grid.ny)]);
  };

  const double c00 = lerp(value(i0, j0, k0), value(i1, j0, k0), tx);
  const double c10 = lerp(value(i0, j1, k0), value(i1, j1, k0), tx);
  const double c01 = lerp(value(i0, j0, k1), value(i1, j0, k1), tx);
  const double c11 = lerp(value(i0, j1, k1), value(i1, j1, k1), tx);
  const double c0 = lerp(c00, c10, ty);
  const double c1 = lerp(c01, c11, ty);
  return finiteOrZero(lerp(c0, c1, tz));
}

Vector3 gridMax(const DenseSDFGrid& grid) {
  return {
      grid.origin.x + grid.spacing.x * static_cast<double>(grid.nx - 1),
      grid.origin.y + grid.spacing.y * static_cast<double>(grid.ny - 1),
      grid.origin.z + grid.spacing.z * static_cast<double>(grid.nz - 1)};
}

Vector3 clampToGrid(const DenseSDFGrid& grid, const Vector3& point) {
  const Vector3 max_corner = gridMax(grid);
  return {
      std::clamp(point.x, grid.origin.x, max_corner.x),
      std::clamp(point.y, grid.origin.y, max_corner.y),
      std::clamp(point.z, grid.origin.z, max_corner.z)};
}

double oneAxisDerivative(
    const DenseSDFGrid& grid,
    const Vector3& point,
    int axis) {
  const double h =
      axis == 0 ? grid.spacing.x : (axis == 1 ? grid.spacing.y : grid.spacing.z);
  if (!(h > 0.0) || !std::isfinite(h)) {
    return 0.0;
  }
  const Vector3 max_corner = gridMax(grid);
  Vector3 minus = point;
  Vector3 plus = point;
  double* minus_axis = axis == 0 ? &minus.x : (axis == 1 ? &minus.y : &minus.z);
  double* plus_axis = axis == 0 ? &plus.x : (axis == 1 ? &plus.y : &plus.z);
  const double lo =
      axis == 0 ? grid.origin.x : (axis == 1 ? grid.origin.y : grid.origin.z);
  const double hi = axis == 0 ? max_corner.x : (axis == 1 ? max_corner.y : max_corner.z);
  *minus_axis = std::max(lo, *minus_axis - h);
  *plus_axis = std::min(hi, *plus_axis + h);
  const double denom = *plus_axis - *minus_axis;
  if (!(std::abs(denom) > 0.0) || !std::isfinite(denom)) {
    return 0.0;
  }
  return finiteOrZero((sampleGrid(grid, plus) - sampleGrid(grid, minus)) / denom);
}

class DenseNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit DenseNativeHandle(std::shared_ptr<DenseSDFGrid> grid)
      : grid_(std::move(grid)) {}

  Scalar sampleDistance(const Vector3& point) const override {
    return sampleGrid(*grid_, point);
  }

  bool canSampleDistance() const override {
    return grid_ && DenseSDFModel::isValidGrid(*grid_);
  }

  bool canSampleGradient() const override {
    return canSampleDistance();
  }

  Vector3 sampleGradient(const Vector3& point) const override {
    const Vector3 clamped = clampToGrid(*grid_, point);
    return {
        oneAxisDerivative(*grid_, clamped, 0),
        oneAxisDerivative(*grid_, clamped, 1),
        oneAxisDerivative(*grid_, clamped, 2)};
  }

  Scalar finiteDifferenceStep() const override {
    if (!grid_) {
      return 1.0e-6;
    }
    return std::max(
        std::min({grid_->spacing.x, grid_->spacing.y, grid_->spacing.z}) * 0.5,
        1.0e-7);
  }

  std::string backendName() const override {
    return DenseSDFModel::backendName();
  }

 private:
  std::shared_ptr<DenseSDFGrid> grid_;
};

}  // namespace

DenseSDFModel::DenseSDFModel()
    : grid_(std::make_shared<DenseSDFGrid>()) {
  setValid(false);
}

DenseSDFModel::DenseSDFModel(DenseSDFGrid grid)
    : grid_(std::make_shared<DenseSDFGrid>(std::move(grid))) {
  if (!isValidGrid(*grid_)) {
    setValid(false);
    return;
  }

  const Vector3 max_corner = gridMax(*grid_);
  setBoundingBox(AABB{grid_->origin, max_corner, true});

  SDFMetadata metadata;
  metadata.model_name = "uniform dense SDF";
  metadata.format_name = "ADASDF_DENSE_SDFBIN_V1";
  metadata.format_version = 1;
  metadata.query_backend = backendName();
  metadata.query_backend_available = true;
  metadata.n_fine_cell = std::max(0, grid_->nx - 1);
  metadata.n_fine_node = grid_->nx;
  metadata.h_fine = std::min({grid_->spacing.x, grid_->spacing.y, grid_->spacing.z});
  metadata.total_dense_memory_mb =
      static_cast<double>(grid_->phi.size() * sizeof(double)) / (1024.0 * 1024.0);
  setMetadata(metadata);
  setMemoryFootprintBytes(
      sizeof(DenseSDFModel) + sizeof(DenseSDFGrid) +
      grid_->phi.size() * sizeof(double));
  setNativeHandle(std::make_shared<DenseNativeHandle>(grid_));
  setValid(true);
}

const DenseSDFGrid& DenseSDFModel::grid() const {
  return *grid_;
}

DenseSDFGrid& DenseSDFModel::grid() {
  return *grid_;
}

bool DenseSDFModel::isValidGrid(const DenseSDFGrid& grid) {
  if (grid.nx < 2 || grid.ny < 2 || grid.nz < 2) {
    return false;
  }
  if (!grid.origin.allFinite() || !grid.spacing.allFinite()) {
    return false;
  }
  if (!(grid.spacing.x > 0.0) || !(grid.spacing.y > 0.0) ||
      !(grid.spacing.z > 0.0)) {
    return false;
  }
  const std::size_t expected =
      static_cast<std::size_t>(grid.nx) * static_cast<std::size_t>(grid.ny) *
      static_cast<std::size_t>(grid.nz);
  if (grid.phi.size() != expected) {
    return false;
  }
  return std::all_of(grid.phi.begin(), grid.phi.end(), [](double value) {
    return std::isfinite(value);
  });
}

const char* DenseSDFModel::backendName() {
  return "core-free uniform DenseSDF backend";
}

}  // namespace adasdf
