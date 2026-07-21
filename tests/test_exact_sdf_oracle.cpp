#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  const std::filesystem::path fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
  if (!read.success) {
    std::cerr << read.error_message << "\n";
    return 1;
  }

  adasdf::ExactSDFOracleOptions options;
  options.bvh_leaf_size = 2;
  adasdf::ExactSDFOracle oracle;
  std::string error;
  if (!oracle.reset(read.mesh, options, &error)) {
    std::cerr << error << "\n";
    return 1;
  }

  const std::vector<adasdf::Vector3> points = {
      {0.5, 0.5, 0.5},
      {1.25, 0.5, 0.5},
      {-0.25, 0.25, 0.25},
      {0.25, 1.25, 0.25}};
  for (const adasdf::Vector3& point : points) {
    const auto exact = oracle.sample(point);
    const auto brute =
        adasdf::BVHSDFSampler::sampleBruteForce(read.mesh, point, true);
    if (!exact.success || !brute.success ||
        std::abs(exact.phi - brute.phi) > 1.0e-9) {
      std::cerr << "oracle signed phi differs from brute-force reference\n";
      return 1;
    }
    const auto repeated = oracle.sample(point);
    if (!repeated.success || !repeated.cache_hit ||
        std::abs(repeated.phi - exact.phi) > 1.0e-15) {
      std::cerr << "oracle did not reuse a repeated spatial node\n";
      return 1;
    }
  }

  const adasdf::ExactSDFOracleStats stats = oracle.stats();
  if (!stats.bvh_built || stats.bvh_triangle_count != read.mesh.triangleCount() ||
      stats.exact_query_requests != points.size() * 2 ||
      stats.unique_exact_queries != points.size() ||
      stats.cache_hits != points.size() ||
      stats.nearest_bvh_node_visits == 0 ||
      stats.nearest_triangle_tests == 0) {
    std::cerr << "oracle counters are incomplete\n";
    return 1;
  }
  const std::size_t brute_distance_tests =
      points.size() * read.mesh.triangleCount();
  if (stats.nearest_triangle_tests >= brute_distance_tests) {
    std::cerr << "BVH nearest search did not reduce triangle traversal\n";
    return 1;
  }

  const adasdf::Vector3 unsigned_point{0.33, 0.44, 1.5};
  const adasdf::ExactSDFOracleStats before_unsigned = oracle.stats();
  const auto unsigned_sample = oracle.sampleUnsignedDistance(unsigned_point);
  const auto unsigned_brute =
      adasdf::BVHSDFSampler::sampleBruteForce(
          read.mesh, unsigned_point, false);
  const adasdf::ExactSDFOracleStats after_unsigned = oracle.stats();
  if (!unsigned_sample.success || !unsigned_brute.success ||
      std::abs(unsigned_sample.phi - unsigned_brute.phi) > 1.0e-9 ||
      after_unsigned.exact_query_requests !=
          before_unsigned.exact_query_requests + 1 ||
      after_unsigned.unique_exact_queries !=
          before_unsigned.unique_exact_queries + 1 ||
      after_unsigned.nearest_bvh_node_visits <=
          before_unsigned.nearest_bvh_node_visits ||
      after_unsigned.sign_bvh_node_visits !=
          before_unsigned.sign_bvh_node_visits ||
      after_unsigned.sign_triangle_tests !=
          before_unsigned.sign_triangle_tests) {
    std::cerr << "unsigned oracle query changed distance or performed sign work\n";
    return 1;
  }

  const auto cached_unsigned = oracle.sampleUnsignedDistance(points.front());
  if (!cached_unsigned.success || !cached_unsigned.cache_hit ||
      std::abs(cached_unsigned.phi - 0.5) > 1.0e-9) {
    std::cerr << "unsigned oracle query did not reuse signed cache data\n";
    return 1;
  }

  adasdf::ExactSDFOracle unsigned_first_oracle;
  if (!unsigned_first_oracle.reset(read.mesh, options, &error)) {
    std::cerr << error << "\n";
    return 1;
  }
  const adasdf::Vector3 bounded_point{0.5, 0.5, 1.5};
  const auto outside_bound =
      unsigned_first_oracle.sampleUnsignedDistance(bounded_point, 0.25);
  const auto inside_bound =
      unsigned_first_oracle.sampleUnsignedDistance(bounded_point, 0.75);
  if (outside_bound.success || !inside_bound.success ||
      std::abs(inside_bound.phi - 0.5) > 1.0e-9) {
    std::cerr << "bounded unsigned oracle query failed\n";
    return 1;
  }
  const adasdf::Vector3 inside{0.5, 0.5, 0.5};
  const auto unsigned_first =
      unsigned_first_oracle.sampleUnsignedDistance(inside);
  const auto signed_after_unsigned = unsigned_first_oracle.sample(inside);
  const auto unsigned_cached =
      unsigned_first_oracle.sampleUnsignedDistance(inside);
  if (!unsigned_first.success || !signed_after_unsigned.success ||
      !(signed_after_unsigned.phi < 0.0) || !unsigned_cached.success ||
      !unsigned_cached.cache_hit ||
      std::abs(unsigned_cached.phi - std::abs(signed_after_unsigned.phi)) >
          1.0e-9) {
    std::cerr << "unsigned cache poisoned a later signed query\n";
    return 1;
  }
  return 0;
}
