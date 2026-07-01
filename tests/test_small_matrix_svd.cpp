#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

bool near(double a, double b, double eps) {
  return std::abs(a - b) <= eps;
}

int main() {
  std::vector<double> rank_one = {
      1.0, 2.0, 3.0, 4.0,
      2.0, 4.0, 6.0, 8.0,
      3.0, 6.0, 9.0, 12.0};
  adasdf::SmallSVDOptions options;
  options.max_rank = 1;
  adasdf::SmallSVDResult svd =
      adasdf::SmallMatrixSVD::compute(rank_one, 3, 4, options);
  if (!svd.success || svd.rank != 1 ||
      svd.reconstruction_max_abs_error > 1.0e-8) {
    std::cerr << "rank-1 SVD reconstruction failed\n";
    return 1;
  }
  if (!std::isfinite(adasdf::SmallMatrixSVD::reconstructValue(svd, 1, 2))) {
    std::cerr << "reconstructValue must be finite\n";
    return 1;
  }

  std::vector<double> full = {
      1.0, 0.0, 2.0,
      0.0, 3.0, 0.0,
      4.0, 0.0, 5.0};
  options.max_rank = 3;
  svd = adasdf::SmallMatrixSVD::compute(full, 3, 3, options);
  if (!svd.success || svd.rank != 3 ||
      svd.reconstruction_max_abs_error > 1.0e-8) {
    std::cerr << "full-rank SVD reconstruction failed\n";
    return 1;
  }
  if (!(svd.S[0] >= svd.S[1] && svd.S[1] >= svd.S[2])) {
    std::cerr << "singular values should be sorted descending\n";
    return 1;
  }

  options.max_rank = 2;
  svd = adasdf::SmallMatrixSVD::compute(full, 3, 3, options);
  if (!svd.success || svd.rank != 2) {
    std::cerr << "fixed truncation rank failed\n";
    return 1;
  }

  full[0] = std::numeric_limits<double>::quiet_NaN();
  svd = adasdf::SmallMatrixSVD::compute(full, 3, 3, options);
  if (svd.success || svd.error_message.empty()) {
    std::cerr << "NaN input should fail\n";
    return 1;
  }
  std::cout << "small matrix SVD passed\n";
  return 0;
}
