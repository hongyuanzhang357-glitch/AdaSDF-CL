#include <adasdf/adasdf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

std::size_t index(int x, int y, int z, int nx, int ny) {
  return static_cast<std::size_t>(x) + static_cast<std::size_t>(nx) *
      (static_cast<std::size_t>(y) +
       static_cast<std::size_t>(ny) * static_cast<std::size_t>(z));
}

}  // namespace

int main() {
  const int nx = 5;
  const int ny = 4;
  const int nz = 3;
  std::vector<double> separable(
      static_cast<std::size_t>(nx * ny * nz), 0.0);
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        separable[index(x, y, z, nx, ny)] =
            (x + 1.0) * (0.5 + y) * (1.0 + 0.25 * z);
      }
    }
  }

  const std::vector<adasdf::TensorCompressionMethod> methods = {
      adasdf::TensorCompressionMethod::Dense,
      adasdf::TensorCompressionMethod::MatrixSVD,
      adasdf::TensorCompressionMethod::HOSVD,
      adasdf::TensorCompressionMethod::TensorTrain,
      adasdf::TensorCompressionMethod::Tucker};
  for (const auto method : methods) {
    adasdf::AdaptiveTensorCompressionOptions options;
    options.method = method;
    options.max_rank = 3;
    options.max_abs_error = 1.0e-8;
    options.near_zero_max_abs_error = 1.0e-8;
    adasdf::AdaptiveTensorCompressionReport report;
    const auto compressed = adasdf::AdaptiveTensorCompressor::compress(
        separable, nx, ny, nz, options, &report);
    const auto decoded = adasdf::AdaptiveTensorCompressor::decode(compressed);
    if (!report.success || decoded.size() != separable.size()) {
      std::cerr << "compression failed for " << adasdf::toString(method) << "\n";
      return 1;
    }
    double max_error = 0.0;
    for (std::size_t i = 0; i < decoded.size(); ++i) {
      max_error = std::max(max_error, std::abs(decoded[i] - separable[i]));
    }
    if (max_error > 1.0e-8 || compressed.compressed_bytes > compressed.original_bytes) {
      std::cerr << "absolute error/size invariant failed for "
                << adasdf::toString(method) << "\n";
      return 1;
    }
    if (method != adasdf::TensorCompressionMethod::Dense &&
        compressed.method == adasdf::TensorCompressionMethod::Dense) {
      std::cerr << "rank-one tensor unexpectedly fell back to Dense for "
                << adasdf::toString(method) << "\n";
      return 1;
    }
    for (const auto coordinates : {
             std::array<double, 3>{0.25, 0.5, 0.75},
             std::array<double, 3>{1.3, 2.1, 1.4},
             std::array<double, 3>{3.7, 0.2, 1.8}}) {
      const int x0 = static_cast<int>(std::floor(coordinates[0]));
      const int y0 = static_cast<int>(std::floor(coordinates[1]));
      const int z0 = static_cast<int>(std::floor(coordinates[2]));
      const int x1 = std::min(x0 + 1, nx - 1);
      const int y1 = std::min(y0 + 1, ny - 1);
      const int z1 = std::min(z0 + 1, nz - 1);
      const double tx = coordinates[0] - x0;
      const double ty = coordinates[1] - y0;
      const double tz = coordinates[2] - z0;
      const auto lerp = [](double a, double b, double t) {
        return a + (b - a) * t;
      };
      const auto node = [&](int x, int y, int z) {
        return decoded[index(x, y, z, nx, ny)];
      };
      const double c00 = lerp(node(x0, y0, z0), node(x1, y0, z0), tx);
      const double c10 = lerp(node(x0, y1, z0), node(x1, y1, z0), tx);
      const double c01 = lerp(node(x0, y0, z1), node(x1, y0, z1), tx);
      const double c11 = lerp(node(x0, y1, z1), node(x1, y1, z1), tx);
      const double expected = lerp(lerp(c00, c10, ty), lerp(c01, c11, ty), tz);
      const double actual = adasdf::AdaptiveTensorCompressor::sampleTrilinear(
          compressed, coordinates[0], coordinates[1], coordinates[2]);
      if (std::abs(actual - expected) > 1.0e-10) {
        std::cerr << "factor-interpolated query disagrees for "
                  << adasdf::toString(method) << "\n";
        return 1;
      }
    }
  }

  std::vector<double> nonseparable = separable;
  for (std::size_t i = 0; i < nonseparable.size(); ++i) {
    nonseparable[i] += 0.01 * std::sin(static_cast<double>(i * i + 3));
  }
  adasdf::AdaptiveTensorCompressionOptions strict;
  strict.method = adasdf::TensorCompressionMethod::Tucker;
  strict.max_rank = 1;
  strict.max_abs_error = 1.0e-12;
  strict.near_zero_max_abs_error = 1.0e-12;
  adasdf::AdaptiveTensorCompressionReport strict_report;
  const auto fallback = adasdf::AdaptiveTensorCompressor::compress(
      nonseparable, nx, ny, nz, strict, &strict_report);
  if (!strict_report.success || !strict_report.dense_fallback ||
      fallback.method != adasdf::TensorCompressionMethod::Dense ||
      fallback.dense != nonseparable) {
    std::cerr << "strict absolute error did not trigger Dense fallback\n";
    return 1;
  }
  return 0;
}
