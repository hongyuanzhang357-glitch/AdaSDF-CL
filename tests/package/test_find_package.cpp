#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  static_assert(adasdf::versionMajor() == 0, "unexpected major version");
  static_assert(adasdf::versionMinor() >= 9, "package test expects v0.9 or newer");

  adasdf::CollisionRequest request;
  request.backend = adasdf::BackendType::CPU;
  request.query_mode = adasdf::QueryMode::Balanced;

  const adasdf::Transform transform = adasdf::Transform::Identity();
  const adasdf::Vector3 point = transform.applyPoint({1.0, 2.0, 3.0});
  const auto backend = adasdf::makeBackend(adasdf::BackendType::CPU);
  adasdf::DemoAdaptiveBuildRequest build_request;
  build_request.use_surrogate = true;
  const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);
  const double phi = build.model->sampleDistance({0.0, 0.0, 0.0});

  std::cout << "AdaSDF-CL package consumer\n";
  std::cout << "Version: " << adasdf::versionString() << "\n";
  std::cout << "Point: " << point << "\n";
  std::cout << "Demo signed distance at origin: " << phi << "\n";
  std::cout << "Demo adaptive blocks: " << build.model->blocks().size() << "\n";
  std::cout << "CPU backend available: "
            << (backend && backend->available() ? "true" : "false") << "\n";
  return backend && std::isfinite(phi) ? 0 : 1;
}
