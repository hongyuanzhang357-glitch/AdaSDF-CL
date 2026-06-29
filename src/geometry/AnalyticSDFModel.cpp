#include "adasdf/geometry/AnalyticSDFModel.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

Scalar absScalar(Scalar value) {
  return std::abs(value);
}

Scalar dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Scalar norm(const Vector3& v) {
  return std::sqrt(dot(v, v));
}

Vector3 normalizedOrFallback(const Vector3& v) {
  const Scalar length = norm(v);
  if (!(length > 1.0e-12) || !v.allFinite()) {
    return {1.0, 0.0, 0.0};
  }
  return v / length;
}

Scalar signNonZero(Scalar value) {
  return value < 0.0 ? -1.0 : 1.0;
}

void validateBox(const AnalyticSDFDescription& description) {
  if (description.shape != AnalyticShapeType::Box) {
    throw std::runtime_error("AnalyticSDFModel currently supports box shapes only.");
  }
  if (!description.center.allFinite() || !description.half_extent.allFinite()) {
    throw std::runtime_error("AnalyticSDFModel box parameters must be finite.");
  }
  if (!(description.half_extent.x > 0.0) ||
      !(description.half_extent.y > 0.0) ||
      !(description.half_extent.z > 0.0)) {
    throw std::runtime_error("AnalyticSDFModel box half extents must be positive.");
  }
}

Vector3 boxLocal(const Vector3& point, const AnalyticSDFDescription& description) {
  return point - description.center;
}

Scalar boxSignedDistance(
    const Vector3& point,
    const AnalyticSDFDescription& description) {
  const Vector3 local = boxLocal(point, description);
  const Vector3 q{
      absScalar(local.x) - description.half_extent.x,
      absScalar(local.y) - description.half_extent.y,
      absScalar(local.z) - description.half_extent.z};
  const Vector3 outside{
      std::max<Scalar>(q.x, 0.0),
      std::max<Scalar>(q.y, 0.0),
      std::max<Scalar>(q.z, 0.0)};
  const Scalar outside_length = norm(outside);
  const Scalar inside = std::min(std::max({q.x, q.y, q.z}), 0.0);
  return outside_length + inside;
}

Vector3 boxGradient(
    const Vector3& point,
    const AnalyticSDFDescription& description) {
  const Vector3 local = boxLocal(point, description);
  const Vector3 q{
      absScalar(local.x) - description.half_extent.x,
      absScalar(local.y) - description.half_extent.y,
      absScalar(local.z) - description.half_extent.z};
  const Vector3 outside{
      q.x > 0.0 ? q.x * signNonZero(local.x) : 0.0,
      q.y > 0.0 ? q.y * signNonZero(local.y) : 0.0,
      q.z > 0.0 ? q.z * signNonZero(local.z) : 0.0};
  if (norm(outside) > 1.0e-12) {
    return normalizedOrFallback(outside);
  }

  // Inside or on a flat face: choose the nearest face. Ties are intentionally
  // deterministic, in x/y/z order, to keep edge and center behavior stable.
  const Scalar dx = description.half_extent.x - absScalar(local.x);
  const Scalar dy = description.half_extent.y - absScalar(local.y);
  const Scalar dz = description.half_extent.z - absScalar(local.z);
  if (dx <= dy && dx <= dz) {
    return {signNonZero(local.x), 0.0, 0.0};
  }
  if (dy <= dx && dy <= dz) {
    return {0.0, signNonZero(local.y), 0.0};
  }
  return {0.0, 0.0, signNonZero(local.z)};
}

class AnalyticNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit AnalyticNativeHandle(AnalyticSDFDescription description)
      : description_(std::move(description)) {}

  Scalar sampleDistance(const Vector3& point) const override {
    return boxSignedDistance(point, description_);
  }

  bool canSampleDistance() const override {
    return true;
  }

  bool canSampleGradient() const override {
    return true;
  }

  Vector3 sampleGradient(const Vector3& point) const override {
    return boxGradient(point, description_);
  }

  Scalar finiteDifferenceStep() const override {
    const Scalar min_extent = std::min(
        {description_.half_extent.x, description_.half_extent.y,
         description_.half_extent.z});
    return std::max(min_extent * 1.0e-5, 1.0e-7);
  }

  std::string backendName() const override {
    return AnalyticSDFModel::backendName();
  }

 private:
  AnalyticSDFDescription description_;
};

}  // namespace

AnalyticSDFModel::AnalyticSDFModel(AnalyticSDFDescription description)
    : description_(std::move(description)) {
  validateBox(description_);

  setBoundingBox(AABB{
      description_.center - description_.half_extent,
      description_.center + description_.half_extent,
      true});

  SDFMetadata metadata;
  metadata.model_name = "analytic demo box";
  metadata.format_name = "ADASDF_DEMO_SDFBIN_V1";
  metadata.query_backend = backendName();
  metadata.format_version = 1;
  metadata.query_backend_available = true;
  metadata.n_fine_cell = 0;
  metadata.n_fine_node = 0;
  metadata.h_fine = 0.0;
  setMetadata(metadata);
  setMemoryFootprintBytes(sizeof(AnalyticSDFModel) + sizeof(AnalyticNativeHandle));
  setNativeHandle(std::make_shared<AnalyticNativeHandle>(description_));
  setValid(true);
}

std::shared_ptr<AnalyticSDFModel> AnalyticSDFModel::createBox(
    const Vector3& center,
    const Vector3& half_extent,
    std::string unit) {
  AnalyticSDFDescription description;
  description.shape = AnalyticShapeType::Box;
  description.center = center;
  description.half_extent = half_extent;
  description.unit = std::move(unit);
  return std::make_shared<AnalyticSDFModel>(std::move(description));
}

std::string AnalyticSDFModel::shapeName() const {
  switch (description_.shape) {
    case AnalyticShapeType::Box:
      return "box";
    case AnalyticShapeType::Sphere:
      return "sphere";
  }
  return "unknown";
}

void AnalyticSDFModel::setSourcePath(std::string source_path) {
  SDFMetadata metadata = this->metadata();
  metadata.source_path = std::move(source_path);
  setMetadata(metadata);
}

const char* AnalyticSDFModel::backendName() {
  return "core-free analytic demo SDF backend";
}

}  // namespace adasdf
