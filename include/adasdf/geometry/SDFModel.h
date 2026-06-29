#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "adasdf/compression/CompressedSDF.h"
#include "adasdf/geometry/CollisionGeometry.h"

namespace adasdf {

struct SDFBlockMetadata {
  BlockId block_id = -1;
  Vector3 origin;
  Vector3 local_min;
  Vector3 local_max;
  VoxelResolution resolution;
  int compression_rank = 0;
  Scalar near_surface_error = 0.0;
  Scalar max_reconstruction_error = 0.0;
};

struct DeviceHandle {
  BackendType backend = BackendType::CPU;
  void* handle = nullptr;
};

struct SDFSample {
  Scalar signed_distance = 0.0;
  Vector3 gradient;
  BlockId block_id = -1;
  bool valid = false;
};

struct SDFMetadata {
  std::string model_name;
  std::string source_path;
  std::string format_name;
  std::string query_backend;

  int format_version = 0;
  int n_fine_cell = 0;
  int n_fine_node = 0;
  Scalar h_fine = 0.0;

  int max_level = 0;
  int final_block_count = 0;
  int active_block_count = 0;
  int near_surface_block_count = 0;
  int sign_change_block_count = 0;

  Scalar near_surface_error = 0.0;
  Scalar near_surface_error_over_h = 0.0;
  Scalar max_reconstruction_error = 0.0;
  double total_low_rank_memory_mb = 0.0;
  double total_dense_memory_mb = 0.0;
  double compression_ratio = 0.0;

  bool query_backend_available = false;
};

class SDFModel : public CollisionGeometry {
 public:
  class NativeHandle {
   public:
    virtual ~NativeHandle() = default;

    virtual Scalar sampleDistance(const Vector3& point) const = 0;
    virtual bool canSampleDistance() const = 0;
    virtual bool canSampleGradient() const {
      return false;
    }
    virtual Vector3 sampleGradient(const Vector3& point) const;
    virtual Scalar finiteDifferenceStep() const = 0;
    virtual std::string backendName() const = 0;
    virtual bool canWriteSDFBin() const {
      return false;
    }
    virtual void writeSDFBin(const std::filesystem::path& path) const;
  };

  SDFModel() = default;
  explicit SDFModel(std::shared_ptr<CompressedSDF> compressed_sdf);

  GeometryKind kind() const override {
    return GeometryKind::SDFModel;
  }

  AABB localAABB() const override;
  std::string debugName() const override;

  bool isValid() const {
    return valid_;
  }

  const AABB& boundingBox() const {
    return bounds_;
  }

  const std::vector<SDFBlockMetadata>& blockMetadata() const {
    return block_metadata_;
  }

  const std::shared_ptr<CompressedSDF>& compressedData() const {
    return compressed_sdf_;
  }

  Scalar nearSurfaceError() const {
    return near_surface_error_;
  }

  std::size_t memoryFootprintBytes() const;

  double sampleDistance(const Vector3& point_world) const;
  Vector3 sampleGradient(const Vector3& point_world) const;
  Vector3 sampleNormal(const Vector3& point_world) const;

  // TODO: connect to sdf::AdaptiveLowRankModel::queryPhi and dense cache paths.
  Scalar signedDistance(const Vector3& local_point) const;

  // TODO: connect to sdf::SDFNormal or analytic finite-difference gradients.
  Vector3 gradient(const Vector3& local_point) const;

  SDFSample sample(const Vector3& local_point) const;

  const SDFMetadata& metadata() const {
    return metadata_;
  }

  bool queryBackendAvailable() const {
    return native_handle_ && native_handle_->canSampleDistance();
  }

  const std::shared_ptr<NativeHandle>& nativeHandle() const {
    return native_handle_;
  }

  const DeviceHandle& deviceHandle() const {
    return device_handle_;
  }

  void setDeviceHandle(DeviceHandle handle) {
    device_handle_ = handle;
  }

  void setBoundingBox(const AABB& bounds);
  void setBlockMetadata(std::vector<SDFBlockMetadata> metadata);
  void setMetadata(const SDFMetadata& metadata);
  void setMemoryFootprintBytes(std::size_t bytes);
  void setNativeHandle(std::shared_ptr<NativeHandle> native_handle);
  void setValid(bool valid);

 private:
  AABB bounds_;
  std::vector<SDFBlockMetadata> block_metadata_;
  std::shared_ptr<CompressedSDF> compressed_sdf_;
  Scalar near_surface_error_ = 0.0;
  std::size_t memory_footprint_bytes_ = 0;
  SDFMetadata metadata_;
  std::shared_ptr<NativeHandle> native_handle_;
  DeviceHandle device_handle_;
  bool valid_ = false;
};

}  // namespace adasdf
