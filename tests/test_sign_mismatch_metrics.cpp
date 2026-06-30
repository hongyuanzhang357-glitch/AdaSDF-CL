#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <string>

int main() {
  const double eps = 1.0e-6;
  if (adasdf::classifySDFSign(-2.0 * eps, eps) !=
      adasdf::SDFSignClass::Inside) {
    std::cerr << "inside sign classification failed\n";
    return 1;
  }
  if (adasdf::classifySDFSign(2.0 * eps, eps) !=
      adasdf::SDFSignClass::Outside) {
    std::cerr << "outside sign classification failed\n";
    return 1;
  }
  if (adasdf::classifySDFSign(0.5 * eps, eps) !=
      adasdf::SDFSignClass::Ambiguous) {
    std::cerr << "ambiguous sign classification failed\n";
    return 1;
  }
  if (!adasdf::isStrictSignMismatch(-1.0, 1.0, eps)) {
    std::cerr << "inside/outside mismatch was not detected\n";
    return 1;
  }
  if (adasdf::isStrictSignMismatch(0.0, 1.0, eps)) {
    std::cerr << "ambiguous sign should not count as strict mismatch\n";
    return 1;
  }

  adasdf::ExpansionQualityReport report;
  report.num_samples = 2;
  report.sign_mismatch_count = 1;
  report.ambiguous_sign_count = 1;
  report.near_surface_sign_mismatch_count = 1;
  if (std::string(adasdf::toString(adasdf::SDFSignClass::Inside)) != "inside") {
    std::cerr << "sign string conversion failed\n";
    return 1;
  }

  std::cout << "sign mismatch metrics passed\n";
  return 0;
}
