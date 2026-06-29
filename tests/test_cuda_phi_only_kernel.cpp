#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {

double maxPhiError(
    const adasdf::BatchQueryOutput& a,
    const adasdf::BatchQueryOutput& b) {
  double error = 0.0;
  const std::size_t count =
      std::min(a.signed_distances.size(), b.signed_distances.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (!std::isfinite(b.signed_distances[i])) {
      return std::numeric_limits<double>::infinity();
    }
    error = std::max(
        error,
        std::abs(a.signed_distances[i] - b.signed_distances[i]));
  }
  return error;
}

std::vector<adasdf::Vector3> testPoints() {
  return {
      {-0.3, -0.2, -0.1},
      {-0.1, 0.1, 0.2},
      {0.0, 0.0, 0.0},
      {0.2, -0.1, 0.1},
      {0.3, 0.2, -0.2}};
}

}  // namespace

int main() {
  if (!adasdf::CudaQueryBackend::isAvailable()) {
    std::cout << "SKIPPED: CUDA query backend unavailable\n";
    return 0;
  }

  const auto model = adasdf::AnalyticSDFModel::createBox();
  const std::vector<adasdf::Vector3> points = testPoints();
  const adasdf::BatchQueryOutput reference =
      adasdf::queryBatchCPU(*model, points);

  adasdf::ExpansionOptions options;
  options.expansion = adasdf::QueryExpansionMode::Global;
  options.global_resolution = 48;
  const adasdf::ExpandedSDF expanded =
      adasdf::SDFExpander::expand(*model, options);

  adasdf::CudaResidentExpandedSDF resident;
  if (!resident.upload(expanded)) {
    std::cerr << "CUDA resident SDF upload failed\n";
    return 1;
  }

  adasdf::CudaQueryWorkspace workspace;
  if (!workspace.ensureCapacity(points.size(), false)) {
    std::cerr << "CUDA phi-only workspace allocation failed\n";
    return 1;
  }
  adasdf::BatchQueryTiming timing;
  const adasdf::BatchQueryOutput phi_only =
      resident.queryBatch(points, true, &workspace, &timing);

  if (phi_only.signed_distances.size() != points.size() ||
      !phi_only.gradients.empty() ||
      !phi_only.normals.empty()) {
    std::cerr << "phi-only output shape is invalid\n";
    return 1;
  }
  if (maxPhiError(reference, phi_only) > 0.05 ||
      timing.kernel_ms <= 0.0 ||
      timing.d2h_results_ms <= 0.0) {
    std::cerr << "phi-only CUDA query is inaccurate or missing timing\n";
    return 1;
  }

  std::cout << "CUDA phi-only kernel aligns with CPU phi\n";
  return 0;
}
