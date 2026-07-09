#include "adasdf/cache/SignQueryCache.h"

namespace adasdf {

SignQueryCache::SignQueryCache(SDFSampleCache* sample_cache)
    : sample_cache_(sample_cache) {}

bool SignQueryCache::find(const QuantizedPointKey& key, int* sign) const {
  if (sample_cache_ == nullptr) {
    return false;
  }
  CachedSDFSample sample;
  if (!sample_cache_->find(key, &sample) || !sample.sign_known) {
    return false;
  }
  if (sign != nullptr) {
    *sign = sample.sign;
  }
  return true;
}

void SignQueryCache::insert(const QuantizedPointKey& key, int sign) {
  if (sample_cache_ == nullptr) {
    return;
  }
  CachedSDFSample sample;
  sample.sign_known = true;
  sample.sign = sign < 0 ? -1 : (sign > 0 ? 1 : 0);
  sample_cache_->insert(key, sample);
}

}  // namespace adasdf
