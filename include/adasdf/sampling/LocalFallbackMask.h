#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace adasdf {

struct LocalFallbackMask {
  int nx = 0;
  int ny = 0;
  int nz = 0;
  std::vector<std::uint8_t> exact_required;

  bool valid() const;
  bool isExactRequired(int i, int j, int k) const;
  std::size_t exactRequiredCount() const;
};

class LocalFallbackMaskBuilder {
 public:
  static LocalFallbackMask build(
      int nx,
      int ny,
      int nz,
      const std::vector<double>& predicted_phi,
      double near_surface_band,
      int halo_exact_layers);
};

}  // namespace adasdf
