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
  adasdf::ExpansionOptions options;
  options.expansion = adasdf::QueryExpansionMode::Global;
  options.global_resolution = 48;
  const adasdf::ExpandedSDF expanded =
      adasdf::SDFExpander::expand(*model, options);

  adasdf::CudaResidentExpandedSDF resident;
  if (!resident.upload(expanded) || resident.deviceMemoryBytes() == 0) {
    std::cerr << "CUDA resident SDF upload failed\n";
    return 1;
  }

  const std::vector<adasdf::Vector3> points = testPoints();
  adasdf::BatchQueryTiming one_shot_timing;
  const adasdf::BatchQueryOutput one_shot =
      resident.queryBatch(points, false, nullptr, &one_shot_timing);

  adasdf::CudaQueryWorkspace workspace;
  if (!workspace.ensureCapacity(points.size(), true) ||
      workspace.capacity() < points.size() ||
      workspace.deviceMemoryBytes() == 0) {
    std::cerr << "CUDA query workspace allocation failed\n";
    return 1;
  }
  adasdf::BatchQueryTiming workspace_timing;
  const adasdf::BatchQueryOutput reused =
      resident.queryBatch(points, false, &workspace, &workspace_timing);

  if (one_shot.signed_distances.size() != reused.signed_distances.size() ||
      one_shot.normals.size() != reused.normals.size()) {
    std::cerr << "workspace query output sizes differ\n";
    return 1;
  }
  if (maxPhiError(one_shot, reused) > 1.0e-12 ||
      workspace_timing.kernel_ms <= 0.0 ||
      workspace_timing.total_ms <= 0.0) {
    std::cerr << "workspace query did not align with one-shot query\n";
    return 1;
  }

  std::cout << "CUDA query workspace aligns with one-shot query\n";
  return 0;
}
