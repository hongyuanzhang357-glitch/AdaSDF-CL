#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::AABB cell;
    cell.valid = true;
    cell.min = {0.0, 0.0, 0.0};
    cell.max = {1.0, 1.0, 1.0};
    const auto exact_phi = [](const adasdf::Vector3& p) {
      return p.x - 0.5;
    };

    const adasdf::LocalExactSubcellProbeResult fixed =
        adasdf::LocalExactSubcellProbe::probe(
            cell,
            {0.25, 0.25, 0.25},
            1,
            -1,
            2,
            exact_phi);
    if (!fixed.fixed_mismatch || fixed.remaining_mismatch ||
        fixed.sign != -1) {
      std::cerr << "subgrid probe failed to fix mismatch\n";
      return 1;
    }

    const adasdf::LocalExactSubcellProbeResult remaining =
        adasdf::LocalExactSubcellProbe::probe(
            cell,
            {0.75, 0.25, 0.25},
            -1,
            -1,
            2,
            exact_phi);
    if (!remaining.remaining_mismatch || remaining.sign != 1) {
      std::cerr << "subgrid probe failed to report remaining mismatch\n";
      return 1;
    }

    std::cout << "local exact subcell probe passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_local_exact_subcell_probe failed: "
              << exc.what() << "\n";
    return 1;
  }
}
