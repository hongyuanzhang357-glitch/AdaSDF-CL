#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  static_assert(adasdf::versionMajor() >= 1, "package test expects v1.0 or newer");

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
  const std::vector<adasdf::Vector3> points = {
      {0.0, 0.0, 0.0},
      {0.75, 0.0, 0.0}};
  adasdf::BatchQueryStats stats;
  const auto batch = adasdf::queryBatchCPU(*build.model, points, &stats);

  std::cout << "AdaSDF-CL package consumer\n";
  std::cout << "Version: " << adasdf::versionString() << "\n";
  std::cout << "Point: " << point << "\n";
  std::cout << "Demo signed distance at origin: " << phi << "\n";
  std::cout << "Demo adaptive blocks: " << build.model->blocks().size() << "\n";
  std::cout << "Batch query points: " << stats.num_points << "\n";
  std::cout << "CUDA batch backend available: "
            << (adasdf::CudaQueryBackend::isAvailable() ? "true" : "false")
            << "\n";
  std::cout << "CPU backend available: "
            << (backend && backend->available() ? "true" : "false") << "\n";
  return backend && std::isfinite(phi) &&
          batch.signed_distances.size() == points.size()
      ? 0
      : 1;
}
