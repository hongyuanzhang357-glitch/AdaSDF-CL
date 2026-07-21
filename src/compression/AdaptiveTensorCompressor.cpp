#include "adasdf/compression/AdaptiveTensorCompressor.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "adasdf/compression/SmallMatrixSVD.h"

namespace adasdf {
namespace {

std::size_t tensorIndex(int x, int y, int z, int nx, int ny) {
  return static_cast<std::size_t>(x) + static_cast<std::size_t>(nx) *
      (static_cast<std::size_t>(y) +
       static_cast<std::size_t>(ny) * static_cast<std::size_t>(z));
}

std::size_t matrixIndex(int row, int col, int cols) {
  return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
      static_cast<std::size_t>(col);
}

std::size_t coreIndex(
    int x,
    int y,
    int z,
    int rx,
    int ry) {
  return static_cast<std::size_t>(x) + static_cast<std::size_t>(rx) *
      (static_cast<std::size_t>(y) +
       static_cast<std::size_t>(ry) * static_cast<std::size_t>(z));
}

bool validInput(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    const AdaptiveTensorCompressionOptions& options) {
  if (nx <= 0 || ny <= 0 || nz <= 0 || options.min_rank <= 0 ||
      options.max_rank < options.min_rank ||
      !(options.max_abs_error >= 0.0) ||
      !std::isfinite(options.max_abs_error) ||
      !(options.near_zero_max_abs_error >= 0.0) ||
      !std::isfinite(options.near_zero_max_abs_error)) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(nx) *
      static_cast<std::size_t>(ny) * static_cast<std::size_t>(nz);
  return phi.size() == expected &&
      std::all_of(phi.begin(), phi.end(), [](double value) {
        return std::isfinite(value);
      });
}

std::vector<double> unfold(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    int mode) {
  const int rows = mode == 0 ? nx : (mode == 1 ? ny : nz);
  const int cols = mode == 0 ? ny * nz : (mode == 1 ? nx * nz : nx * ny);
  std::vector<double> matrix(
      static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), 0.0);
  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        int row = x;
        int col = y * nz + z;
        if (mode == 1) {
          row = y;
          col = x * nz + z;
        } else if (mode == 2) {
          row = z;
          col = x * ny + y;
        }
        matrix[matrixIndex(row, col, cols)] =
            phi[tensorIndex(x, y, z, nx, ny)];
      }
    }
  }
  return matrix;
}

SmallSVDResult computeSVD(
    const std::vector<double>& matrix,
    int rows,
    int cols,
    int rank) {
  SmallSVDOptions options;
  options.max_rank = std::max(1, rank);
  options.max_jacobi_iterations = 200;
  return SmallMatrixSVD::compute(matrix, rows, cols, options);
}

std::uint64_t vectorBytes(const std::vector<double>& values) {
  return static_cast<std::uint64_t>(values.size()) * sizeof(double);
}

std::uint64_t payloadBytes(const CompressedTensor3D& tensor) {
  return vectorBytes(tensor.dense) + vectorBytes(tensor.matrix_u) +
      vectorBytes(tensor.matrix_s) + vectorBytes(tensor.matrix_vt) +
      vectorBytes(tensor.factor_x) + vectorBytes(tensor.factor_y) +
      vectorBytes(tensor.factor_z) + vectorBytes(tensor.core) +
      vectorBytes(tensor.tt_core_1) + vectorBytes(tensor.tt_core_2) +
      vectorBytes(tensor.tt_core_3);
}

CompressedTensor3D denseTensor(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    TensorCompressionMethod requested,
    const std::string& note) {
  CompressedTensor3D out;
  out.requested_method = requested;
  out.method = TensorCompressionMethod::Dense;
  out.nx = nx;
  out.ny = ny;
  out.nz = nz;
  out.rank_x = nx;
  out.rank_y = ny;
  out.rank_z = nz;
  out.original_bytes = vectorBytes(phi);
  out.dense = phi;
  out.compressed_bytes = vectorBytes(out.dense);
  out.decoded_peak_bytes = out.original_bytes;
  out.dense_fallback = requested != TensorCompressionMethod::Dense;
  out.note = note;
  return out;
}

CompressedTensor3D matrixSVD(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    int rank,
    TensorCompressionMethod requested) {
  CompressedTensor3D out;
  out.requested_method = requested;
  out.method = TensorCompressionMethod::MatrixSVD;
  out.nx = nx;
  out.ny = ny;
  out.nz = nz;
  const std::vector<double> matrix = unfold(phi, nx, ny, nz, 0);
  const SmallSVDResult svd = computeSVD(matrix, nx, ny * nz, rank);
  if (!svd.success) {
    return out;
  }
  out.rank_x = svd.rank;
  out.rank_y = 1;
  out.rank_z = 1;
  out.matrix_u = svd.U;
  out.matrix_s = svd.S;
  out.matrix_vt = svd.Vt;
  out.original_bytes = vectorBytes(phi);
  out.compressed_bytes = payloadBytes(out);
  out.decoded_peak_bytes = out.original_bytes + out.compressed_bytes;
  return out;
}

CompressedTensor3D tucker(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    int rank,
    TensorCompressionMethod method) {
  CompressedTensor3D out;
  out.requested_method = method;
  out.method = method;
  out.nx = nx;
  out.ny = ny;
  out.nz = nz;
  const SmallSVDResult sx = computeSVD(unfold(phi, nx, ny, nz, 0), nx, ny * nz, rank);
  const SmallSVDResult sy = computeSVD(unfold(phi, nx, ny, nz, 1), ny, nx * nz, rank);
  const SmallSVDResult sz = computeSVD(unfold(phi, nx, ny, nz, 2), nz, nx * ny, rank);
  if (!sx.success || !sy.success || !sz.success) {
    return out;
  }
  out.rank_x = sx.rank;
  out.rank_y = sy.rank;
  out.rank_z = sz.rank;
  out.factor_x = sx.U;
  out.factor_y = sy.U;
  out.factor_z = sz.U;
  out.core.assign(
      static_cast<std::size_t>(out.rank_x) *
          static_cast<std::size_t>(out.rank_y) *
          static_cast<std::size_t>(out.rank_z),
      0.0);
  for (int rz = 0; rz < out.rank_z; ++rz) {
    for (int ry = 0; ry < out.rank_y; ++ry) {
      for (int rx = 0; rx < out.rank_x; ++rx) {
        double value = 0.0;
        for (int z = 0; z < nz; ++z) {
          for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
              value += phi[tensorIndex(x, y, z, nx, ny)] *
                  out.factor_x[matrixIndex(x, rx, out.rank_x)] *
                  out.factor_y[matrixIndex(y, ry, out.rank_y)] *
                  out.factor_z[matrixIndex(z, rz, out.rank_z)];
            }
          }
        }
        out.core[coreIndex(rx, ry, rz, out.rank_x, out.rank_y)] = value;
      }
    }
  }
  out.original_bytes = vectorBytes(phi);
  out.compressed_bytes = payloadBytes(out);
  out.decoded_peak_bytes = out.original_bytes + out.compressed_bytes;
  return out;
}

CompressedTensor3D tensorTrain(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    int rank) {
  CompressedTensor3D out;
  out.requested_method = TensorCompressionMethod::TensorTrain;
  out.method = TensorCompressionMethod::TensorTrain;
  out.nx = nx;
  out.ny = ny;
  out.nz = nz;
  const SmallSVDResult first =
      computeSVD(unfold(phi, nx, ny, nz, 0), nx, ny * nz, rank);
  if (!first.success) {
    return out;
  }
  out.rank_x = first.rank;
  out.tt_core_1 = first.U;
  std::vector<double> remainder(
      static_cast<std::size_t>(first.rank * ny) *
          static_cast<std::size_t>(nz),
      0.0);
  for (int r = 0; r < first.rank; ++r) {
    for (int y = 0; y < ny; ++y) {
      for (int z = 0; z < nz; ++z) {
        remainder[matrixIndex(r * ny + y, z, nz)] =
            first.S[static_cast<std::size_t>(r)] *
            first.Vt[matrixIndex(r, y * nz + z, ny * nz)];
      }
    }
  }
  const SmallSVDResult second =
      computeSVD(remainder, first.rank * ny, nz, rank);
  if (!second.success) {
    return {};
  }
  out.rank_y = second.rank;
  out.rank_z = second.rank;
  out.tt_core_2 = second.U;
  out.tt_core_3.assign(
      static_cast<std::size_t>(second.rank) * static_cast<std::size_t>(nz),
      0.0);
  for (int r = 0; r < second.rank; ++r) {
    for (int z = 0; z < nz; ++z) {
      out.tt_core_3[matrixIndex(r, z, nz)] =
          second.S[static_cast<std::size_t>(r)] *
          second.Vt[matrixIndex(r, z, nz)];
    }
  }
  out.original_bytes = vectorBytes(phi);
  out.compressed_bytes = payloadBytes(out);
  out.decoded_peak_bytes = out.original_bytes + out.compressed_bytes;
  return out;
}

struct ErrorMetrics {
  double max_abs = 0.0;
  double near_zero_max_abs = 0.0;
  std::size_t near_zero_sign_mismatch = 0;
};

bool signMismatch(double reference, double reconstructed) {
  return std::abs(reference) > 1.0e-14 &&
      std::abs(reconstructed) > 1.0e-14 &&
      ((reference < 0.0) != (reconstructed < 0.0));
}

ErrorMetrics errorMetrics(
    const std::vector<double>& reference,
    const CompressedTensor3D& tensor,
    double near_zero_band) {
  ErrorMetrics out;
  for (int z = 0; z < tensor.nz; ++z) {
    for (int y = 0; y < tensor.ny; ++y) {
      for (int x = 0; x < tensor.nx; ++x) {
        const std::size_t index = tensorIndex(x, y, z, tensor.nx, tensor.ny);
        const double reconstructed = AdaptiveTensorCompressor::value(tensor, x, y, z);
        const double error = std::abs(reconstructed - reference[index]);
        out.max_abs = std::max(out.max_abs, error);
        if (std::abs(reference[index]) <= near_zero_band) {
          out.near_zero_max_abs = std::max(out.near_zero_max_abs, error);
          out.near_zero_sign_mismatch +=
              signMismatch(reference[index], reconstructed) ? 1u : 0u;
        }
      }
    }
  }
  return out;
}

bool acceptable(
    const ErrorMetrics& metrics,
    const AdaptiveTensorCompressionOptions& options) {
  return metrics.max_abs <= options.max_abs_error &&
      metrics.near_zero_max_abs <= options.near_zero_max_abs_error &&
      (!options.protect_near_zero_sign ||
       metrics.near_zero_sign_mismatch == 0);
}

}  // namespace

const char* toString(TensorCompressionMethod method) {
  switch (method) {
    case TensorCompressionMethod::Dense:
      return "Dense";
    case TensorCompressionMethod::MatrixSVD:
      return "MatrixSVD";
    case TensorCompressionMethod::HOSVD:
      return "HOSVD";
    case TensorCompressionMethod::TensorTrain:
      return "TT";
    case TensorCompressionMethod::Tucker:
      return "Tucker";
  }
  return "Dense";
}

CompressedTensor3D AdaptiveTensorCompressor::compress(
    const std::vector<double>& phi,
    int nx,
    int ny,
    int nz,
    const AdaptiveTensorCompressionOptions& options,
    AdaptiveTensorCompressionReport* report_out) {
  AdaptiveTensorCompressionReport report;
  report.requested_method = options.method;
  report.original_bytes = vectorBytes(phi);
  if (!validInput(phi, nx, ny, nz, options)) {
    report.error_message = "invalid adaptive tensor compression input";
    if (report_out != nullptr) {
      *report_out = report;
    }
    return {};
  }
  if (options.method == TensorCompressionMethod::Dense) {
    CompressedTensor3D out = denseTensor(phi, nx, ny, nz, options.method, "dense requested");
    report.success = true;
    report.selected_method = out.method;
    report.selected_rank_x = out.rank_x;
    report.selected_rank_y = out.rank_y;
    report.selected_rank_z = out.rank_z;
    report.compressed_bytes = out.compressed_bytes;
    if (report_out != nullptr) {
      *report_out = report;
    }
    return out;
  }

  CompressedTensor3D selected;
  ErrorMetrics selected_metrics;
  const int rank_limit = std::max(1, std::min(
      options.max_rank, std::max({nx, ny, nz})));
  for (int rank = options.min_rank; rank <= rank_limit; ++rank) {
    ++report.attempted_rank_count;
    CompressedTensor3D candidate;
    switch (options.method) {
      case TensorCompressionMethod::MatrixSVD:
        candidate = matrixSVD(phi, nx, ny, nz, rank, options.method);
        break;
      case TensorCompressionMethod::HOSVD:
      case TensorCompressionMethod::Tucker:
        candidate = tucker(phi, nx, ny, nz, rank, options.method);
        break;
      case TensorCompressionMethod::TensorTrain:
        candidate = tensorTrain(phi, nx, ny, nz, rank);
        break;
      case TensorCompressionMethod::Dense:
        break;
    }
    if (candidate.compressed_bytes == 0) {
      continue;
    }
    const ErrorMetrics metrics =
        errorMetrics(phi, candidate, options.near_zero_band);
    candidate.tolerance = options.max_abs_error;
    candidate.actual_max_abs_error = metrics.max_abs;
    candidate.actual_near_zero_max_abs_error = metrics.near_zero_max_abs;
    candidate.near_zero_sign_mismatch_count =
        metrics.near_zero_sign_mismatch;
    selected = std::move(candidate);
    selected_metrics = metrics;
    if (acceptable(metrics, options)) {
      break;
    }
  }

  const bool error_failed = selected.compressed_bytes == 0 ||
      !acceptable(selected_metrics, options);
  const bool size_failed = selected.compressed_bytes >= report.original_bytes;
  if (error_failed || (options.fallback_dense_if_larger && size_failed)) {
    selected = denseTensor(
        phi,
        nx,
        ny,
        nz,
        options.method,
        error_failed ? "dense fallback: absolute error or sign guard failed"
                     : "dense fallback: compressed payload is not smaller");
    selected_metrics = {};
  } else {
    selected.note = "adaptive rank selected by absolute error and sign guard";
  }

  report.success = true;
  report.selected_method = selected.method;
  report.selected_rank_x = selected.rank_x;
  report.selected_rank_y = selected.rank_y;
  report.selected_rank_z = selected.rank_z;
  report.max_abs_error = selected.actual_max_abs_error;
  report.near_zero_max_abs_error = selected.actual_near_zero_max_abs_error;
  report.near_zero_sign_mismatch_count =
      selected.near_zero_sign_mismatch_count;
  report.compressed_bytes = selected.compressed_bytes;
  report.dense_fallback = selected.dense_fallback;
  if (report_out != nullptr) {
    *report_out = report;
  }
  return selected;
}

double AdaptiveTensorCompressor::value(
    const CompressedTensor3D& tensor,
    int ix,
    int iy,
    int iz) {
  if (ix < 0 || iy < 0 || iz < 0 || ix >= tensor.nx || iy >= tensor.ny ||
      iz >= tensor.nz) {
    return 0.0;
  }
  if (tensor.method == TensorCompressionMethod::Dense) {
    const std::size_t index = tensorIndex(ix, iy, iz, tensor.nx, tensor.ny);
    return index < tensor.dense.size() ? tensor.dense[index] : 0.0;
  }
  if (tensor.method == TensorCompressionMethod::MatrixSVD) {
    double result = 0.0;
    const int col = iy * tensor.nz + iz;
    for (int r = 0; r < tensor.rank_x; ++r) {
      result += tensor.matrix_u[matrixIndex(ix, r, tensor.rank_x)] *
          tensor.matrix_s[static_cast<std::size_t>(r)] *
          tensor.matrix_vt[matrixIndex(r, col, tensor.ny * tensor.nz)];
    }
    return result;
  }
  if (tensor.method == TensorCompressionMethod::TensorTrain) {
    double result = 0.0;
    for (int r1 = 0; r1 < tensor.rank_x; ++r1) {
      for (int r2 = 0; r2 < tensor.rank_y; ++r2) {
        result += tensor.tt_core_1[matrixIndex(ix, r1, tensor.rank_x)] *
            tensor.tt_core_2[matrixIndex(
                r1 * tensor.ny + iy, r2, tensor.rank_y)] *
            tensor.tt_core_3[matrixIndex(r2, iz, tensor.nz)];
      }
    }
    return result;
  }
  double result = 0.0;
  for (int rz = 0; rz < tensor.rank_z; ++rz) {
    for (int ry = 0; ry < tensor.rank_y; ++ry) {
      for (int rx = 0; rx < tensor.rank_x; ++rx) {
        result += tensor.core[coreIndex(
                      rx, ry, rz, tensor.rank_x, tensor.rank_y)] *
            tensor.factor_x[matrixIndex(ix, rx, tensor.rank_x)] *
            tensor.factor_y[matrixIndex(iy, ry, tensor.rank_y)] *
            tensor.factor_z[matrixIndex(iz, rz, tensor.rank_z)];
      }
    }
  }
  return result;
}

double AdaptiveTensorCompressor::sampleTrilinear(
    const CompressedTensor3D& tensor,
    double x,
    double y,
    double z) {
  if (tensor.nx <= 0 || tensor.ny <= 0 || tensor.nz <= 0 ||
      !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
    return 0.0;
  }
  x = std::clamp(x, 0.0, static_cast<double>(tensor.nx - 1));
  y = std::clamp(y, 0.0, static_cast<double>(tensor.ny - 1));
  z = std::clamp(z, 0.0, static_cast<double>(tensor.nz - 1));
  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const int z0 = static_cast<int>(std::floor(z));
  const int x1 = std::min(x0 + 1, tensor.nx - 1);
  const int y1 = std::min(y0 + 1, tensor.ny - 1);
  const int z1 = std::min(z0 + 1, tensor.nz - 1);
  const double tx = x - static_cast<double>(x0);
  const double ty = y - static_cast<double>(y0);
  const double tz = z - static_cast<double>(z0);
  const auto interpolate = [](double a, double b, double t) {
    return a + (b - a) * t;
  };
  const auto cornerInterpolation = [&]() {
    const double c00 = interpolate(
        value(tensor, x0, y0, z0), value(tensor, x1, y0, z0), tx);
    const double c10 = interpolate(
        value(tensor, x0, y1, z0), value(tensor, x1, y1, z0), tx);
    const double c01 = interpolate(
        value(tensor, x0, y0, z1), value(tensor, x1, y0, z1), tx);
    const double c11 = interpolate(
        value(tensor, x0, y1, z1), value(tensor, x1, y1, z1), tx);
    return interpolate(
        interpolate(c00, c10, ty), interpolate(c01, c11, ty), tz);
  };
  if (tensor.method == TensorCompressionMethod::Dense) {
    return cornerInterpolation();
  }
  if (tensor.method == TensorCompressionMethod::MatrixSVD) {
    double result = 0.0;
    const int columns = tensor.ny * tensor.nz;
    for (int r = 0; r < tensor.rank_x; ++r) {
      const double u = interpolate(
          tensor.matrix_u[matrixIndex(x0, r, tensor.rank_x)],
          tensor.matrix_u[matrixIndex(x1, r, tensor.rank_x)],
          tx);
      const double v0 = interpolate(
          tensor.matrix_vt[matrixIndex(r, y0 * tensor.nz + z0, columns)],
          tensor.matrix_vt[matrixIndex(r, y1 * tensor.nz + z0, columns)],
          ty);
      const double v1 = interpolate(
          tensor.matrix_vt[matrixIndex(r, y0 * tensor.nz + z1, columns)],
          tensor.matrix_vt[matrixIndex(r, y1 * tensor.nz + z1, columns)],
          ty);
      result += u * tensor.matrix_s[static_cast<std::size_t>(r)] *
          interpolate(v0, v1, tz);
    }
    return result;
  }
  if (tensor.method == TensorCompressionMethod::TensorTrain) {
    double result = 0.0;
    for (int r1 = 0; r1 < tensor.rank_x; ++r1) {
      const double core1 = interpolate(
          tensor.tt_core_1[matrixIndex(x0, r1, tensor.rank_x)],
          tensor.tt_core_1[matrixIndex(x1, r1, tensor.rank_x)],
          tx);
      for (int r2 = 0; r2 < tensor.rank_y; ++r2) {
        const double core2 = interpolate(
            tensor.tt_core_2[matrixIndex(
                r1 * tensor.ny + y0, r2, tensor.rank_y)],
            tensor.tt_core_2[matrixIndex(
                r1 * tensor.ny + y1, r2, tensor.rank_y)],
            ty);
        const double core3 = interpolate(
            tensor.tt_core_3[matrixIndex(r2, z0, tensor.nz)],
            tensor.tt_core_3[matrixIndex(r2, z1, tensor.nz)],
            tz);
        result += core1 * core2 * core3;
      }
    }
    return result;
  }
  double result = 0.0;
  for (int rz = 0; rz < tensor.rank_z; ++rz) {
    const double factor_z = interpolate(
        tensor.factor_z[matrixIndex(z0, rz, tensor.rank_z)],
        tensor.factor_z[matrixIndex(z1, rz, tensor.rank_z)],
        tz);
    for (int ry = 0; ry < tensor.rank_y; ++ry) {
      const double factor_y = interpolate(
          tensor.factor_y[matrixIndex(y0, ry, tensor.rank_y)],
          tensor.factor_y[matrixIndex(y1, ry, tensor.rank_y)],
          ty);
      for (int rx = 0; rx < tensor.rank_x; ++rx) {
        const double factor_x = interpolate(
            tensor.factor_x[matrixIndex(x0, rx, tensor.rank_x)],
            tensor.factor_x[matrixIndex(x1, rx, tensor.rank_x)],
            tx);
        result += tensor.core[coreIndex(
                      rx, ry, rz, tensor.rank_x, tensor.rank_y)] *
            factor_x * factor_y * factor_z;
      }
    }
  }
  return result;
}

std::vector<double> AdaptiveTensorCompressor::decode(
    const CompressedTensor3D& tensor) {
  if (tensor.nx <= 0 || tensor.ny <= 0 || tensor.nz <= 0) {
    return {};
  }
  std::vector<double> out(
      static_cast<std::size_t>(tensor.nx) *
          static_cast<std::size_t>(tensor.ny) *
          static_cast<std::size_t>(tensor.nz),
      0.0);
  for (int z = 0; z < tensor.nz; ++z) {
    for (int y = 0; y < tensor.ny; ++y) {
      for (int x = 0; x < tensor.nx; ++x) {
        out[tensorIndex(x, y, z, tensor.nx, tensor.ny)] =
            value(tensor, x, y, z);
      }
    }
  }
  return out;
}

}  // namespace adasdf
