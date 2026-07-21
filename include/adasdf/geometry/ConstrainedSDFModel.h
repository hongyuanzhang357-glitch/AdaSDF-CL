#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/io/ConstrainedSDFBin.h"

namespace adasdf {

enum class ConstrainedSDFQueryStorage {
  CompressedDirect,
  DecodedCache,
};

const char* toString(ConstrainedSDFQueryStorage storage);

class ConstrainedSDFModel final : public SDFModel {
 public:
  explicit ConstrainedSDFModel(
      ConstrainedSDFBinReader reader,
      ConstrainedSDFQueryStorage storage =
          ConstrainedSDFQueryStorage::CompressedDirect);

  static std::shared_ptr<ConstrainedSDFModel> load(
      const std::filesystem::path& path,
      ConstrainedSDFQueryStorage storage =
          ConstrainedSDFQueryStorage::CompressedDirect);

  const ConstrainedSDFBinReader& reader() const { return *reader_; }
  ConstrainedSDFQueryStorage queryStorage() const { return storage_; }
  static const char* backendName();

 private:
  std::shared_ptr<ConstrainedSDFBinReader> reader_;
  ConstrainedSDFQueryStorage storage_ =
      ConstrainedSDFQueryStorage::CompressedDirect;
};

}  // namespace adasdf
