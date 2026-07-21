#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace adasdf {

enum class TensorCompressionMethod {
  Dense,
  MatrixSVD,
  HOSVD,
  TensorTrain,
  Tucker,
};

const char* toString(TensorCompressionMethod method);

struct AdaptiveTensorCompressionOptions {
  TensorCompressionMethod method = TensorCompressionMethod::HOSVD;
  int min_rank = 1;
  int max_rank = 8;
  double max_abs_error = 1.0e-3;
  double near_zero_band = 1.0e-2;
  double near_zero_max_abs_error = 5.0e-4;
  bool protect_near_zero_sign = true;
  bool fallback_dense_if_larger = true;
};

struct CompressedTensor3D {
  TensorCompressionMethod requested_method = TensorCompressionMethod::Dense;
  TensorCompressionMethod method = TensorCompressionMethod::Dense;
  int nx = 0;
  int ny = 0;
  int nz = 0;
  int rank_x = 0;
  int rank_y = 0;
  int rank_z = 0;
  double tolerance = 0.0;
  double actual_max_abs_error = 0.0;
  double actual_near_zero_max_abs_error = 0.0;
  std::size_t near_zero_sign_mismatch_count = 0;
  std::uint64_t original_bytes = 0;
  std::uint64_t compressed_bytes = 0;
  std::uint64_t decoded_peak_bytes = 0;
  bool dense_fallback = false;
  std::string note;

  std::vector<double> dense;
  std::vector<double> matrix_u;
  std::vector<double> matrix_s;
  std::vector<double> matrix_vt;
  std::vector<double> factor_x;
  std::vector<double> factor_y;
  std::vector<double> factor_z;
  std::vector<double> core;
  std::vector<double> tt_core_1;
  std::vector<double> tt_core_2;
  std::vector<double> tt_core_3;
};

struct AdaptiveTensorCompressionReport {
  bool success = false;
  std::string error_message;
  TensorCompressionMethod requested_method = TensorCompressionMethod::Dense;
  TensorCompressionMethod selected_method = TensorCompressionMethod::Dense;
  int attempted_rank_count = 0;
  int selected_rank_x = 0;
  int selected_rank_y = 0;
  int selected_rank_z = 0;
  double max_abs_error = 0.0;
  double near_zero_max_abs_error = 0.0;
  std::size_t near_zero_sign_mismatch_count = 0;
  std::uint64_t original_bytes = 0;
  std::uint64_t compressed_bytes = 0;
  bool dense_fallback = false;
};

class AdaptiveTensorCompressor {
 public:
  static CompressedTensor3D compress(
      const std::vector<double>& phi,
      int nx,
      int ny,
      int nz,
      const AdaptiveTensorCompressionOptions& options,
      AdaptiveTensorCompressionReport* report = nullptr);

  static double value(
      const CompressedTensor3D& tensor,
      int ix,
      int iy,
      int iz);

  static double sampleTrilinear(
      const CompressedTensor3D& tensor,
      double x,
      double y,
      double z);

  static std::vector<double> decode(const CompressedTensor3D& tensor);
};

}  // namespace adasdf
