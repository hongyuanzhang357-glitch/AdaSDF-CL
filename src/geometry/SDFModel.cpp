#include "adasdf/geometry/SDFModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace adasdf {
namespace {

double norm(const Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool finite(const Vector3& v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

}  // namespace

void SDFModel::NativeHandle::writeSDFBin(const std::filesystem::path&) const {
  throw std::runtime_error("This SDFModel native handle cannot write .sdfbin.");
}

Vector3 SDFModel::NativeHandle::sampleGradient(const Vector3&) const {
  throw std::runtime_error("This SDFModel native handle cannot sample gradients.");
}

SDFModel::SDFModel(std::shared_ptr<CompressedSDF> compressed_sdf)
    : compressed_sdf_(std::move(compressed_sdf)) {
  valid_ = static_cast<bool>(compressed_sdf_);
}

AABB SDFModel::localAABB() const {
  return bounds_;
}

std::string SDFModel::debugName() const {
  return metadata_.model_name.empty() ? "SDFModel" : metadata_.model_name;
}

std::size_t SDFModel::memoryFootprintBytes() const {
  if (memory_footprint_bytes_ > 0) {
    return memory_footprint_bytes_;
  }
  return compressed_sdf_ ? compressed_sdf_->memoryFootprintBytes() : 0;
}

double SDFModel::sampleDistance(const Vector3& point_world) const {
  if (!native_handle_ || !native_handle_->canSampleDistance()) {
    throw std::runtime_error(
        "SDFModel has no available query backend; load a supported .sdfbin "
        "with ADASDF_CL_USE_EXISTING_CORE=ON.");
  }
  return native_handle_->sampleDistance(point_world);
}

Vector3 SDFModel::sampleGradient(const Vector3& point_world) const {
  if (!native_handle_ || !native_handle_->canSampleDistance()) {
    throw std::runtime_error(
        "SDFModel has no available query backend for gradient sampling.");
  }
  if (native_handle_->canSampleGradient()) {
    const Vector3 gradient = native_handle_->sampleGradient(point_world);
    if (!finite(gradient)) {
      throw std::runtime_error("SDFModel native gradient produced non-finite values.");
    }
    return gradient;
  }

  double h = native_handle_->finiteDifferenceStep();
  if (!(h > 0.0) || !std::isfinite(h)) {
    const Vector3 diag = bounds_.max - bounds_.min;
    h = std::max(norm(diag) * 1.0e-5, 1.0e-6);
  }

  const Vector3 dx{h, 0.0, 0.0};
  const Vector3 dy{0.0, h, 0.0};
  const Vector3 dz{0.0, 0.0, h};

  const double gx =
      (sampleDistance(point_world + dx) - sampleDistance(point_world - dx)) / (2.0 * h);
  const double gy =
      (sampleDistance(point_world + dy) - sampleDistance(point_world - dy)) / (2.0 * h);
  const double gz =
      (sampleDistance(point_world + dz) - sampleDistance(point_world - dz)) / (2.0 * h);

  Vector3 gradient{gx, gy, gz};
  if (!finite(gradient)) {
    throw std::runtime_error("SDFModel finite-difference gradient produced non-finite values.");
  }
  return gradient;
}

Vector3 SDFModel::sampleNormal(const Vector3& point_world) const {
  const Vector3 gradient = sampleGradient(point_world);
  const double length = norm(gradient);
  if (!(length > 1.0e-12)) {
    return Vector3::Zero();
  }
  return {gradient.x / length, gradient.y / length, gradient.z / length};
}

Scalar SDFModel::signedDistance(const Vector3& local_point) const {
  return sampleDistance(local_point);
}

Vector3 SDFModel::gradient(const Vector3& local_point) const {
  return sampleGradient(local_point);
}

SDFSample SDFModel::sample(const Vector3& local_point) const {
  SDFSample result;
  result.signed_distance = sampleDistance(local_point);
  result.gradient = sampleGradient(local_point);
  result.valid = true;
  return result;
}

void SDFModel::setBoundingBox(const AABB& bounds) {
  bounds_ = bounds;
}

void SDFModel::setBlockMetadata(std::vector<SDFBlockMetadata> metadata) {
  block_metadata_ = std::move(metadata);
}

void SDFModel::setMetadata(const SDFMetadata& metadata) {
  metadata_ = metadata;
  near_surface_error_ = metadata.near_surface_error;
  if (metadata.total_low_rank_memory_mb > 0.0) {
    memory_footprint_bytes_ =
        static_cast<std::size_t>(metadata.total_low_rank_memory_mb * 1024.0 * 1024.0);
  }
}

void SDFModel::setMemoryFootprintBytes(std::size_t bytes) {
  memory_footprint_bytes_ = bytes;
}

void SDFModel::setNativeHandle(std::shared_ptr<NativeHandle> native_handle) {
  native_handle_ = std::move(native_handle);
  metadata_.query_backend_available =
      native_handle_ && native_handle_->canSampleDistance();
  metadata_.query_backend =
      native_handle_ ? native_handle_->backendName() : std::string{};
}

void SDFModel::setValid(bool valid) {
  valid_ = valid;
}

}  // namespace adasdf
