#include "adasdf/backend/CudaQueryBackend.h"

#include <cuda_runtime.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace adasdf {
namespace cuda_detail {
namespace {

struct DeviceVec3 {
  double x;
  double y;
  double z;
};

__device__ double absDevice(double value) {
  return value < 0.0 ? -value : value;
}

__device__ double signNonZeroDevice(double value) {
  return value < 0.0 ? -1.0 : 1.0;
}

__device__ double normDevice(const DeviceVec3& value) {
  return sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

__device__ DeviceVec3 normalizedOrFallbackDevice(const DeviceVec3& value) {
  const double length = normDevice(value);
  if (!(length > 1.0e-12)) {
    return {1.0, 0.0, 0.0};
  }
  return {value.x / length, value.y / length, value.z / length};
}

__global__ void boxSdfKernel(
    const DeviceVec3* points,
    std::size_t count,
    DeviceVec3 center,
    DeviceVec3 half_extent,
    double* phi,
    DeviceVec3* normals) {
  const std::size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count) {
    return;
  }

  const DeviceVec3 local{
      points[i].x - center.x,
      points[i].y - center.y,
      points[i].z - center.z};
  const DeviceVec3 q{
      absDevice(local.x) - half_extent.x,
      absDevice(local.y) - half_extent.y,
      absDevice(local.z) - half_extent.z};
  const DeviceVec3 outside{
      q.x > 0.0 ? q.x : 0.0,
      q.y > 0.0 ? q.y : 0.0,
      q.z > 0.0 ? q.z : 0.0};
  const double max_q = fmax(q.x, fmax(q.y, q.z));
  phi[i] = normDevice(outside) + fmin(max_q, 0.0);

  const DeviceVec3 outside_gradient{
      q.x > 0.0 ? q.x * signNonZeroDevice(local.x) : 0.0,
      q.y > 0.0 ? q.y * signNonZeroDevice(local.y) : 0.0,
      q.z > 0.0 ? q.z * signNonZeroDevice(local.z) : 0.0};
  if (normDevice(outside_gradient) > 1.0e-12) {
    normals[i] = normalizedOrFallbackDevice(outside_gradient);
    return;
  }

  const double dx = half_extent.x - absDevice(local.x);
  const double dy = half_extent.y - absDevice(local.y);
  const double dz = half_extent.z - absDevice(local.z);
  if (dx <= dy && dx <= dz) {
    normals[i] = {signNonZeroDevice(local.x), 0.0, 0.0};
  } else if (dy <= dx && dy <= dz) {
    normals[i] = {0.0, signNonZeroDevice(local.y), 0.0};
  } else {
    normals[i] = {0.0, 0.0, signNonZeroDevice(local.z)};
  }
}

DeviceVec3 toDeviceVec3(const Vector3& value) {
  return {value.x, value.y, value.z};
}

Vector3 toVector3(const DeviceVec3& value) {
  return {value.x, value.y, value.z};
}

void checkCuda(cudaError_t error, const char* operation) {
  if (error != cudaSuccess) {
    throw std::runtime_error(
        std::string(operation) + " failed: " + cudaGetErrorString(error));
  }
}

}  // namespace

bool isRuntimeAvailable() {
  int count = 0;
  const cudaError_t error = cudaGetDeviceCount(&count);
  if (error != cudaSuccess) {
    cudaGetLastError();
    return false;
  }
  return count > 0;
}

BatchQueryOutput queryAnalyticBoxOnCuda(
    const Vector3& center,
    const Vector3& half_extent,
    const BatchQueryInput& input) {
  BatchQueryOutput output;
  output.signed_distances.resize(input.points.size());
  output.gradients.resize(input.points.size());
  output.normals.resize(input.points.size());
  if (input.points.empty()) {
    return output;
  }

  std::vector<DeviceVec3> host_points;
  host_points.reserve(input.points.size());
  for (const Vector3& point : input.points) {
    host_points.push_back(toDeviceVec3(point));
  }

  DeviceVec3* device_points = nullptr;
  double* device_phi = nullptr;
  DeviceVec3* device_normals = nullptr;
  const std::size_t point_bytes = host_points.size() * sizeof(DeviceVec3);
  const std::size_t phi_bytes = output.signed_distances.size() * sizeof(double);

  try {
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_points), point_bytes),
        "cudaMalloc(points)");
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_phi), phi_bytes),
        "cudaMalloc(phi)");
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_normals), point_bytes),
        "cudaMalloc(normals)");
    checkCuda(
        cudaMemcpy(
            device_points,
            host_points.data(),
            point_bytes,
            cudaMemcpyHostToDevice),
        "cudaMemcpy(points)");

    constexpr int block_size = 256;
    const int grid_size =
        static_cast<int>((host_points.size() + block_size - 1) / block_size);
    boxSdfKernel<<<grid_size, block_size>>>(
        device_points,
        host_points.size(),
        toDeviceVec3(center),
        toDeviceVec3(half_extent),
        device_phi,
        device_normals);
    checkCuda(cudaGetLastError(), "boxSdfKernel launch");
    checkCuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

    std::vector<DeviceVec3> host_normals(input.points.size());
    checkCuda(
        cudaMemcpy(
            output.signed_distances.data(),
            device_phi,
            phi_bytes,
            cudaMemcpyDeviceToHost),
        "cudaMemcpy(phi)");
    checkCuda(
        cudaMemcpy(
            host_normals.data(),
            device_normals,
            point_bytes,
            cudaMemcpyDeviceToHost),
        "cudaMemcpy(normals)");

    for (std::size_t i = 0; i < host_normals.size(); ++i) {
      output.gradients[i] = toVector3(host_normals[i]);
      output.normals[i] = output.gradients[i];
    }
  } catch (...) {
    cudaFree(device_points);
    cudaFree(device_phi);
    cudaFree(device_normals);
    throw;
  }

  cudaFree(device_points);
  cudaFree(device_phi);
  cudaFree(device_normals);
  return output;
}

}  // namespace cuda_detail
}  // namespace adasdf
