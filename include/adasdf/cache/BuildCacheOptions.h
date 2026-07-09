#pragma once

#include <cstddef>
#include <string>

namespace adasdf {

enum class BuildCacheScope {
  Off,
  Block,
  Global,
};

const char* toString(BuildCacheScope scope);
bool parseBuildCacheScope(const std::string& value, BuildCacheScope* scope);

struct BuildCacheOptions {
  BuildCacheScope sample_cache = BuildCacheScope::Block;
  BuildCacheScope corner_cache = BuildCacheScope::Block;
  bool sign_cache = true;
  bool distance_cache = true;
  bool marker_cache = true;
  std::size_t cache_max_entries = 0;
  double cache_quantization_epsilon = 1e-12;
  bool report_cache_stats = false;

  bool anySampleCacheEnabled() const {
    return sample_cache != BuildCacheScope::Off;
  }

  bool anyCornerCacheEnabled() const {
    return corner_cache != BuildCacheScope::Off;
  }

  bool anyCacheEnabled() const {
    return anySampleCacheEnabled() || anyCornerCacheEnabled() || sign_cache ||
           distance_cache || marker_cache;
  }
};

}  // namespace adasdf
