#pragma once

#include <string>
#include <vector>

namespace adasdf {

struct SmallSVDOptions {
  int max_rank = 8;
  double singular_value_tolerance = 1.0e-12;
  int max_jacobi_iterations = 100;
  double jacobi_tolerance = 1.0e-12;
};

struct SmallSVDResult {
  bool success = false;
  std::string error_message;

  int rows = 0;
  int cols = 0;
  int rank = 0;

  std::vector<double> U;
  std::vector<double> S;
  std::vector<double> Vt;

  double reconstruction_max_abs_error = 0.0;
  double reconstruction_rms_error = 0.0;
};

class SmallMatrixSVD {
 public:
  static SmallSVDResult compute(
      const std::vector<double>& matrix,
      int rows,
      int cols,
      const SmallSVDOptions& options = SmallSVDOptions{});

  static double reconstructValue(
      const SmallSVDResult& svd,
      int row,
      int col);
};

}  // namespace adasdf
