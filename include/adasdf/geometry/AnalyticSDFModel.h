#pragma once

#include <memory>
#include <string>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

enum class AnalyticShapeType {
  Box,
  Sphere
};

struct AnalyticSDFDescription {
  AnalyticShapeType shape = AnalyticShapeType::Box;
  Vector3 center = Vector3::Zero();
  Vector3 half_extent{0.5, 0.5, 0.5};
  std::string unit = "m";
};

class AnalyticSDFModel final : public SDFModel {
 public:
  explicit AnalyticSDFModel(AnalyticSDFDescription description);

  static std::shared_ptr<AnalyticSDFModel> createBox(
      const Vector3& center = Vector3::Zero(),
      const Vector3& half_extent = Vector3{0.5, 0.5, 0.5},
      std::string unit = "m");

  static std::shared_ptr<AnalyticSDFModel> createSphere(
      const Vector3& center = Vector3::Zero(),
      Scalar radius = 0.5,
      std::string unit = "m");

  AnalyticShapeType shapeType() const {
    return description_.shape;
  }

  const Vector3& center() const {
    return description_.center;
  }

  const Vector3& halfExtent() const {
    return description_.half_extent;
  }

  const std::string& unit() const {
    return description_.unit;
  }

  std::string shapeName() const;
  void setSourcePath(std::string source_path);

  static const char* backendName();

 private:
  AnalyticSDFDescription description_;
};

}  // namespace adasdf
