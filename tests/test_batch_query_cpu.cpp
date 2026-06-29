#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

double vectorError(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool checkModel(const adasdf::SDFModel& model) {
  adasdf::PointCloudGeneratorOptions options;
  options.num_points = 128;
  options.seed = 42;
  options.distribution = adasdf::BenchmarkPointDistribution::Mixed;
  const std::vector<adasdf::Vector3> points =
      adasdf::generateBenchmarkPoints(options);

  adasdf::BatchQueryStats stats;
  const adasdf::BatchQueryOutput batch =
      adasdf::queryBatchCPU(model, points, &stats);
  if (batch.signed_distances.size() != points.size() ||
      batch.gradients.size() != points.size() ||
      batch.normals.size() != points.size() ||
      stats.num_points != points.size() ||
      stats.backend.find("CPU batch query") == std::string::npos) {
    return false;
  }
  for (std::size_t i = 0; i < points.size(); ++i) {
    const double phi = model.sampleDistance(points[i]);
    const adasdf::Vector3 gradient = model.sampleGradient(points[i]);
    if (!std::isfinite(batch.signed_distances[i]) ||
        std::abs(phi - batch.signed_distances[i]) > 1.0e-12 ||
        vectorError(gradient, batch.gradients[i]) > 1.0e-12 ||
        !batch.normals[i].allFinite()) {
      return false;
    }
  }
  return stats.total_ms >= 0.0 && stats.ns_per_query >= 0.0;
}

}  // namespace

int main() {
  const auto analytic = adasdf::AnalyticSDFModel::createBox();
  if (!checkModel(*analytic)) {
    std::cerr << "CPU batch query failed for analytic box\n";
    return 1;
  }

  adasdf::DemoAdaptiveBuildRequest request;
  request.use_surrogate = true;
  const auto demo = adasdf::DemoAdaptiveSDFBuilder::build(request);
  if (!demo.model || !checkModel(*demo.model)) {
    std::cerr << "CPU batch query failed for demo adaptive box\n";
    return 1;
  }

  std::cout << "CPU batch query aligns with scalar query path\n";
  return 0;
}
