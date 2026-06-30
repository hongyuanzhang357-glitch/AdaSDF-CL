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
  adasdf::BatchQueryTiming phi_timing;
  adasdf::BatchQueryOutput phi;
  resident.queryBatchInto(
      points,
      adasdf::QueryOutputMode::PhiOnly,
      &workspace,
      &phi,
      &phi_timing);
  if (phi.signed_distances.size() != points.size() ||
      !phi.gradients.empty() ||
      !phi.normals.empty() ||
      maxPhiError(reference, phi) > 0.05 ||
      phi_timing.kernel_ms <= 0.0) {
    std::cerr << "CUDA phi-only output mode failed\n";
    return 1;
  }

  adasdf::BatchQueryTiming full_timing;
  adasdf::BatchQueryOutput full;
  resident.queryBatchInto(
      points,
      adasdf::QueryOutputMode::PhiAndNormal,
      &workspace,
      &full,
      &full_timing);
  if (full.signed_distances.size() != points.size() ||
      full.gradients.size() != points.size() ||
      full.normals.size() != points.size() ||
      maxPhiError(reference, full) > 0.05 ||
      full_timing.kernel_ms <= 0.0) {
    std::cerr << "CUDA phi+normal output mode failed\n";
    return 1;
  }

  std::cout << "CUDA output modes align with CPU phi\n";
  return 0;
}
