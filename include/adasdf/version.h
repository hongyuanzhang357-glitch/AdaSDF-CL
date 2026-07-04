#pragma once

#ifndef ADASDF_CL_VERSION_MAJOR
#define ADASDF_CL_VERSION_MAJOR 1
#endif

#ifndef ADASDF_CL_VERSION_MINOR
#define ADASDF_CL_VERSION_MINOR 16
#endif

#ifndef ADASDF_CL_VERSION_PATCH
#define ADASDF_CL_VERSION_PATCH 0
#endif

#ifndef ADASDF_CL_VERSION_SUFFIX
#define ADASDF_CL_VERSION_SUFFIX "alpha.2"
#endif

namespace adasdf {

inline constexpr int versionMajor() {
  return ADASDF_CL_VERSION_MAJOR;
}

inline constexpr int versionMinor() {
  return ADASDF_CL_VERSION_MINOR;
}

inline constexpr int versionPatch() {
  return ADASDF_CL_VERSION_PATCH;
}

inline constexpr const char* versionSuffix() {
  return ADASDF_CL_VERSION_SUFFIX;
}

inline constexpr const char* versionString() {
  return "1.16.0-alpha.2";
}

}  // namespace adasdf
