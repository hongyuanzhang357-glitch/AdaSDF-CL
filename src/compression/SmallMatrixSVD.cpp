#include "adasdf/compression/SmallMatrixSVD.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace adasdf {
namespace {

std::size_t idx(int row, int col, int cols) {
  return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
         static_cast<std::size_t>(col);
}

bool allFinite(const std::vector<double>& values) {
  return std::all_of(values.begin(), values.end(), [](double value) {
    return std::isfinite(value);
  });
}

std::vector<double> identity(int n) {
  std::vector<double> values(static_cast<std::size_t>(n) *
                                 static_cast<std::size_t>(n),
                             0.0);
  for (int i = 0; i < n; ++i) {
    values[idx(i, i, n)] = 1.0;
  }
  return values;
}

void jacobiEigenSymmetric(
    std::vector<double>& a,
    std::vector<double>& eigenvectors,
    int n,
    int max_iterations,
    double tolerance) {
  eigenvectors = identity(n);
  if (n <= 1) {
    return;
  }

  for (int iter = 0; iter < max_iterations; ++iter) {
    int p = 0;
    int q = 1;
    double max_offdiag = std::abs(a[idx(p, q, n)]);
    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        const double value = std::abs(a[idx(i, j, n)]);
        if (value > max_offdiag) {
          max_offdiag = value;
          p = i;
          q = j;
        }
      }
    }
    if (max_offdiag <= tolerance) {
      break;
    }

    const double app = a[idx(p, p, n)];
    const double aqq = a[idx(q, q, n)];
    const double apq = a[idx(p, q, n)];
    if (std::abs(apq) <= tolerance) {
      continue;
    }
    const double tau = (aqq - app) / (2.0 * apq);
    const double t =
        (tau >= 0.0 ? 1.0 : -1.0) /
        (std::abs(tau) + std::sqrt(1.0 + tau * tau));
    const double c = 1.0 / std::sqrt(1.0 + t * t);
    const double s = t * c;

    for (int k = 0; k < n; ++k) {
      if (k == p || k == q) {
        continue;
      }
      const double akp = a[idx(k, p, n)];
      const double akq = a[idx(k, q, n)];
      a[idx(k, p, n)] = c * akp - s * akq;
      a[idx(p, k, n)] = a[idx(k, p, n)];
      a[idx(k, q, n)] = s * akp + c * akq;
      a[idx(q, k, n)] = a[idx(k, q, n)];
    }

    const double app_new = c * c * app - 2.0 * s * c * apq + s * s * aqq;
    const double aqq_new = s * s * app + 2.0 * s * c * apq + c * c * aqq;
    a[idx(p, p, n)] = app_new;
    a[idx(q, q, n)] = aqq_new;
    a[idx(p, q, n)] = 0.0;
    a[idx(q, p, n)] = 0.0;

    for (int k = 0; k < n; ++k) {
      const double vkp = eigenvectors[idx(k, p, n)];
      const double vkq = eigenvectors[idx(k, q, n)];
      eigenvectors[idx(k, p, n)] = c * vkp - s * vkq;
      eigenvectors[idx(k, q, n)] = s * vkp + c * vkq;
    }
  }
}

}  // namespace

SmallSVDResult SmallMatrixSVD::compute(
    const std::vector<double>& matrix,
    int rows,
    int cols,
    const SmallSVDOptions& options) {
  SmallSVDResult result;
  result.rows = rows;
  result.cols = cols;

  if (rows <= 0 || cols <= 0) {
    result.error_message = "SmallMatrixSVD requires positive dimensions.";
    return result;
  }
  if (matrix.size() !=
      static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols)) {
    result.error_message = "SmallMatrixSVD input size does not match dimensions.";
    return result;
  }
  if (!allFinite(matrix)) {
    result.error_message = "SmallMatrixSVD input contains NaN or Inf.";
    return result;
  }
  const int rank_limit = std::max(
      0,
      std::min({options.max_rank, rows, cols}));
  if (rank_limit == 0) {
    result.error_message = "SmallMatrixSVD max_rank produced zero rank.";
    return result;
  }

  std::vector<double> gram(static_cast<std::size_t>(rows) *
                               static_cast<std::size_t>(rows),
                           0.0);
  for (int i = 0; i < rows; ++i) {
    for (int j = i; j < rows; ++j) {
      double value = 0.0;
      for (int k = 0; k < cols; ++k) {
        value += matrix[idx(i, k, cols)] * matrix[idx(j, k, cols)];
      }
      gram[idx(i, j, rows)] = value;
      gram[idx(j, i, rows)] = value;
    }
  }

  std::vector<double> eigenvectors;
  jacobiEigenSymmetric(
      gram,
      eigenvectors,
      rows,
      std::max(1, options.max_jacobi_iterations),
      std::max(0.0, options.jacobi_tolerance));

  std::vector<int> order(static_cast<std::size_t>(rows));
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    return gram[idx(a, a, rows)] > gram[idx(b, b, rows)];
  });

  std::vector<double> u_columns;
  for (int out_rank = 0; out_rank < rank_limit; ++out_rank) {
    const int eig = order[static_cast<std::size_t>(out_rank)];
    const double lambda = std::max(0.0, gram[idx(eig, eig, rows)]);
    const double sigma = std::sqrt(lambda);
    if (!(sigma > options.singular_value_tolerance) ||
        !std::isfinite(sigma)) {
      continue;
    }

    result.S.push_back(sigma);
    for (int row = 0; row < rows; ++row) {
      u_columns.push_back(eigenvectors[idx(row, eig, rows)]);
    }

    for (int col = 0; col < cols; ++col) {
      double value = 0.0;
      for (int row = 0; row < rows; ++row) {
        value += matrix[idx(row, col, cols)] *
                 eigenvectors[idx(row, eig, rows)];
      }
      result.Vt.push_back(value / sigma);
    }
  }

  result.rank = static_cast<int>(result.S.size());
  if (result.rank == 0) {
    result.error_message =
        "SmallMatrixSVD found no singular values above tolerance.";
    return result;
  }
  result.U.assign(
      static_cast<std::size_t>(rows) * static_cast<std::size_t>(result.rank),
      0.0);
  for (int r = 0; r < result.rank; ++r) {
    for (int row = 0; row < rows; ++row) {
      result.U[static_cast<std::size_t>(row) *
                   static_cast<std::size_t>(result.rank) +
               static_cast<std::size_t>(r)] =
          u_columns[static_cast<std::size_t>(r) *
                        static_cast<std::size_t>(rows) +
                    static_cast<std::size_t>(row)];
    }
  }

  double max_abs_error = 0.0;
  double sum_sq = 0.0;
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      const double diff =
          reconstructValue(result, row, col) - matrix[idx(row, col, cols)];
      max_abs_error = std::max(max_abs_error, std::abs(diff));
      sum_sq += diff * diff;
    }
  }
  result.reconstruction_max_abs_error = max_abs_error;
  result.reconstruction_rms_error = std::sqrt(
      sum_sq /
      static_cast<double>(static_cast<std::size_t>(rows) *
                          static_cast<std::size_t>(cols)));
  result.success = true;
  return result;
}

double SmallMatrixSVD::reconstructValue(
    const SmallSVDResult& svd,
    int row,
    int col) {
  if (row < 0 || col < 0 || row >= svd.rows || col >= svd.cols ||
      svd.rank <= 0) {
    return 0.0;
  }
  double value = 0.0;
  for (int r = 0; r < svd.rank; ++r) {
    const double u = svd.U[static_cast<std::size_t>(row) *
                               static_cast<std::size_t>(svd.rank) +
                           static_cast<std::size_t>(r)];
    const double s = svd.S[static_cast<std::size_t>(r)];
    const double vt = svd.Vt[static_cast<std::size_t>(r) *
                                 static_cast<std::size_t>(svd.cols) +
                             static_cast<std::size_t>(col)];
    value += u * s * vt;
  }
  return std::isfinite(value) ? value : 0.0;
}

}  // namespace adasdf
