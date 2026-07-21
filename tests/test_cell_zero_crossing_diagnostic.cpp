#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::CellZeroCrossingInput same_sign;
    same_sign.exact_corner_phi.assign(8, 0.2);
    same_sign.exact_center_phi = -0.1;
    const adasdf::CellZeroCrossingResult same_sign_result =
        adasdf::CellZeroCrossingDiagnostic::diagnose(same_sign);
    if (!same_sign_result.likely_zero_crossing_inside_cell ||
        same_sign_result.category !=
            "surface_crossing_with_same_sign_corners") {
      std::cerr << "same-sign corner zero-crossing category failed\n";
      return 1;
    }

    adasdf::CellZeroCrossingInput mixed;
    mixed.exact_corner_phi = {-0.1, 0.1, 0.1, 0.1,
                              0.1, 0.1, 0.1, 0.1};
    const adasdf::CellZeroCrossingResult mixed_result =
        adasdf::CellZeroCrossingDiagnostic::diagnose(mixed);
    if (!mixed_result.exact_corners_mixed_sign ||
        mixed_result.category != "surface_crossing_with_mixed_corners") {
      std::cerr << "mixed corner zero-crossing category failed\n";
      return 1;
    }

    adasdf::CellZeroCrossingInput suspicious;
    suspicious.block_lookup_suspicious = true;
    suspicious.exact_corner_phi.assign(8, 0.2);
    const adasdf::CellZeroCrossingResult suspicious_result =
        adasdf::CellZeroCrossingDiagnostic::diagnose(suspicious);
    if (suspicious_result.category != "block_lookup_suspicious") {
      std::cerr << "block lookup category failed\n";
      return 1;
    }

    std::cout << "cell zero-crossing diagnostic passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_cell_zero_crossing_diagnostic failed: "
              << exc.what() << "\n";
    return 1;
  }
}
