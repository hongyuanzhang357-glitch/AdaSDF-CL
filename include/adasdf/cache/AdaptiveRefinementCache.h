#pragma once

#include "adasdf/cache/BuildCacheOptions.h"
#include "adasdf/cache/BuildCacheStats.h"
#include "adasdf/cache/QuantizedPointKey.h"
#include "adasdf/cache/SDFSampleCache.h"

namespace adasdf {

class AdaptiveRefinementCache {
 public:
  explicit AdaptiveRefinementCache(const BuildCacheOptions& options);

  void setQuantization(const QuantizationOptions& quantization);
  bool findSample(const Vector3& p, int level, CachedSDFSample* out);
  void insertSample(const Vector3& p, int level, const CachedSDFSample& sample);
  void beginBlock(int block_id);
  void endBlock(int block_id);
  const BuildCacheStats& stats() const;
  BuildCacheStats snapshotStats() const;

 private:
  BuildCacheOptions options_;
  QuantizationOptions quantization_;
  int current_block_id_ = -1;
  SDFSampleCache block_cache_;
  SDFSampleCache global_cache_;
  mutable BuildCacheStats stats_;
};

}  // namespace adasdf
