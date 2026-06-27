#include "adasdf/io/ContactOnlySDFBin.h"

#include <stdexcept>

namespace adasdf {

ContactOnlySDFBin ContactOnlySDFBin::read(const std::filesystem::path&) {
  throw std::runtime_error("ContactOnlySDFBin::read is not implemented in v0.2.");
}

void ContactOnlySDFBin::write(const std::filesystem::path&) const {
  throw std::runtime_error("ContactOnlySDFBin::write is not implemented in v0.2.");
}

}  // namespace adasdf
