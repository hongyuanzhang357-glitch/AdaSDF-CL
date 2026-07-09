#include "adasdf/cache/QuantizedPointKey.h"

#include <cmath>
#include <cstdint>

namespace adasdf {

bool QuantizedPointKey::operator==(
    const QuantizedPointKey& other) const noexcept {
  return ix == other.ix && iy == other.iy && iz == other.iz &&
         level == other.level;
}

namespace {

std::size_t mix(std::size_t seed, std::int64_t value) {
  const std::size_t x = static_cast<std::size_t>(value);
  seed ^= x + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
  return seed;
}

std::int64_t quantizeAxis(double value, double origin, double spacing) {
  const double safe_spacing = spacing > 0.0 ? spacing : 1.0;
  return static_cast<std::int64_t>(
      std::llround((value - origin) / safe_spacing));
}

}  // namespace

std::size_t QuantizedPointKeyHash::operator()(
    const QuantizedPointKey& key) const noexcept {
  std::size_t seed = 0;
  seed = mix(seed, key.ix);
  seed = mix(seed, key.iy);
  seed = mix(seed, key.iz);
  seed = mix(seed, key.level);
  return seed;
}

QuantizedPointKey QuantizedPointKeyBuilder::fromPoint(
    const Vector3& p,
    int level,
    const QuantizationOptions& options) {
  const double spacing =
      std::max(options.epsilon > 0.0 ? options.epsilon : 1e-12,
               options.spacing > 0.0 ? options.spacing : 1.0);
  QuantizedPointKey key;
  key.ix = quantizeAxis(p.x, options.origin.x, spacing);
  key.iy = quantizeAxis(p.y, options.origin.y, spacing);
  key.iz = quantizeAxis(p.z, options.origin.z, spacing);
  key.level = options.include_level ? level : 0;
  return key;
}

}  // namespace adasdf
