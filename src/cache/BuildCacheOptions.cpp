#include "adasdf/cache/BuildCacheOptions.h"

namespace adasdf {

const char* toString(BuildCacheScope scope) {
  switch (scope) {
    case BuildCacheScope::Off:
      return "off";
    case BuildCacheScope::Block:
      return "block";
    case BuildCacheScope::Global:
      return "global";
  }
  return "off";
}

bool parseBuildCacheScope(const std::string& value, BuildCacheScope* scope) {
  if (scope == nullptr) {
    return false;
  }
  if (value == "off" || value == "none" || value == "false" ||
      value == "0") {
    *scope = BuildCacheScope::Off;
    return true;
  }
  if (value == "block" || value == "local") {
    *scope = BuildCacheScope::Block;
    return true;
  }
  if (value == "global") {
    *scope = BuildCacheScope::Global;
    return true;
  }
  return false;
}

}  // namespace adasdf
