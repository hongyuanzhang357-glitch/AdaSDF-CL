#include "adasdf/runtime/cuda/CudaActiveBlockBuffer.h"

#include <cuda_runtime.h>

#include <chrono>
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

struct ActiveBlockBufferHandle {
  CudaActiveBlockMetadata* device_metadata = nullptr;
  double* device_values = nullptr;
  int block_count = 0;
  int value_count = 0;
  std::size_t memory_bytes = 0;
};

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

bool checkCuda(cudaError_t error, std::string* message) {
  if (error == cudaSuccess) {
    return true;
  }
  if (message != nullptr) {
    *message = cudaGetErrorString(error);
  }
  return false;
}

void releaseHandle(ActiveBlockBufferHandle* handle) {
  if (handle == nullptr) {
    return;
  }
  if (handle->device_metadata != nullptr) {
    cudaFree(handle->device_metadata);
  }
  if (handle->device_values != nullptr) {
    cudaFree(handle->device_values);
  }
  handle->device_metadata = nullptr;
  handle->device_values = nullptr;
  handle->block_count = 0;
  handle->value_count = 0;
  handle->memory_bytes = 0;
}

__device__ double clampDevice(double value, double lo, double hi) {
  return fmin(hi, fmax(lo, value));
}

__device__ bool containsDevice(
    const CudaActiveBlockMetadata& block,
    const DeviceVec3& point) {
  const double eps = 1.0e-12;
  return point.x >= block.min_x - eps && point.x <= block.max_x + eps &&
         point.y >= block.min_y - eps && point.y <= block.max_y + eps &&
         point.z >= block.min_z - eps && point.z <= block.max_z + eps;
}

__device__ double blockDiagonalDevice(const CudaActiveBlockMetadata& block) {
  const double dx = block.max_x - block.min_x;
  const double dy = block.max_y - block.min_y;
  const double dz = block.max_z - block.min_z;
  return sqrt(dx * dx + dy * dy + dz * dz);
}

__device__ int chooseBlockDevice(
    const CudaActiveBlockMetadata* metadata,
    int block_count,
    const DeviceVec3& point) {
  int best = -1;
  int best_level = -2147483647;
  double best_diag = 1.0e300;
  int best_id = 2147483647;
  for (int i = 0; i < block_count; ++i) {
    const CudaActiveBlockMetadata& block = metadata[i];
    if (!containsDevice(block, point)) {
      continue;
    }
    const double diag = blockDiagonalDevice(block);
    if (block.level > best_level ||
        (block.level == best_level && diag < best_diag) ||
        (block.level == best_level && diag == best_diag &&
         block.block_id < best_id)) {
      best = i;
      best_level = block.level;
      best_diag = diag;
      best_id = block.block_id;
    }
  }
  return best;
}

__device__ unsigned long long valueIndexDevice(
    int i,
    int j,
    int k,
    int nx,
    int ny) {
  return static_cast<unsigned long long>(i) +
         static_cast<unsigned long long>(nx) *
             (static_cast<unsigned long long>(j) +
              static_cast<unsigned long long>(ny) *
                  static_cast<unsigned long long>(k));
}

__device__ bool sampleBlockDevice(
    const CudaActiveBlockMetadata& block,
    const double* values,
    const DeviceVec3& point,
    double* phi) {
  if (block.nx < 2 || block.ny < 2 || block.nz < 2 ||
      block.spacing_x <= 0.0 || block.spacing_y <= 0.0 ||
      block.spacing_z <= 0.0 || block.value_count <= 0) {
    return false;
  }

  const double sx = clampDevice(
      (point.x - block.origin_x) / block.spacing_x,
      0.0,
      static_cast<double>(block.nx - 1));
  const double sy = clampDevice(
      (point.y - block.origin_y) / block.spacing_y,
      0.0,
      static_cast<double>(block.ny - 1));
  const double sz = clampDevice(
      (point.z - block.origin_z) / block.spacing_z,
      0.0,
      static_cast<double>(block.nz - 1));

  int i = static_cast<int>(floor(sx));
  int j = static_cast<int>(floor(sy));
  int k = static_cast<int>(floor(sz));
  i = max(0, min(i, block.nx - 2));
  j = max(0, min(j, block.ny - 2));
  k = max(0, min(k, block.nz - 2));

  const double tx = clampDevice(sx - static_cast<double>(i), 0.0, 1.0);
  const double ty = clampDevice(sy - static_cast<double>(j), 0.0, 1.0);
  const double tz = clampDevice(sz - static_cast<double>(k), 0.0, 1.0);
  const unsigned long long base =
      static_cast<unsigned long long>(block.value_offset);
  const int nx = block.nx;
  const int ny = block.ny;

  const double c000 = values[base + valueIndexDevice(i, j, k, nx, ny)];
  const double c100 = values[base + valueIndexDevice(i + 1, j, k, nx, ny)];
  const double c010 = values[base + valueIndexDevice(i, j + 1, k, nx, ny)];
  const double c110 = values[base + valueIndexDevice(i + 1, j + 1, k, nx, ny)];
  const double c001 = values[base + valueIndexDevice(i, j, k + 1, nx, ny)];
  const double c101 = values[base + valueIndexDevice(i + 1, j, k + 1, nx, ny)];
  const double c011 = values[base + valueIndexDevice(i, j + 1, k + 1, nx, ny)];
  const double c111 =
      values[base + valueIndexDevice(i + 1, j + 1, k + 1, nx, ny)];

  const double c00 = (1.0 - tx) * c000 + tx * c100;
  const double c10 = (1.0 - tx) * c010 + tx * c110;
  const double c01 = (1.0 - tx) * c001 + tx * c101;
  const double c11 = (1.0 - tx) * c011 + tx * c111;
  const double c0 = (1.0 - ty) * c00 + ty * c10;
  const double c1 = (1.0 - ty) * c01 + ty * c11;
  *phi = (1.0 - tz) * c0 + tz * c1;
  return true;
}

__device__ DeviceVec3 normalFromBlockDevice(
    const CudaActiveBlockMetadata& block,
    const double* values,
    const DeviceVec3& point) {
  const double hx = fmax(block.spacing_x * 0.5, 1.0e-7);
  const double hy = fmax(block.spacing_y * 0.5, 1.0e-7);
  const double hz = fmax(block.spacing_z * 0.5, 1.0e-7);
  double mx = 0.0;
  double px = 0.0;
  double my = 0.0;
  double py = 0.0;
  double mz = 0.0;
  double pz = 0.0;
  sampleBlockDevice(block, values, {point.x - hx, point.y, point.z}, &mx);
  sampleBlockDevice(block, values, {point.x + hx, point.y, point.z}, &px);
  sampleBlockDevice(block, values, {point.x, point.y - hy, point.z}, &my);
  sampleBlockDevice(block, values, {point.x, point.y + hy, point.z}, &py);
  sampleBlockDevice(block, values, {point.x, point.y, point.z - hz}, &mz);
  sampleBlockDevice(block, values, {point.x, point.y, point.z + hz}, &pz);
  DeviceVec3 n{
      (px - mx) / (2.0 * hx),
      (py - my) / (2.0 * hy),
      (pz - mz) / (2.0 * hz)};
  const double length = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  if (!(length > 1.0e-20)) {
    return {0.0, 0.0, 0.0};
  }
  return {n.x / length, n.y / length, n.z / length};
}

__global__ void cudaActiveBlockQueryPhiKernel(
    const CudaActiveBlockMetadata* metadata,
    int block_count,
    const double* values,
    const DeviceVec3* points,
    const double* radii,
    std::size_t sample_count,
    double threshold,
    double* phi,
    double* effective_phi,
    int* colliding,
    int* block_ids,
    int* fallback_needed) {
  const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= sample_count) {
    return;
  }
  const DeviceVec3 point = points[index];
  const int block_index = chooseBlockDevice(metadata, block_count, point);
  if (block_index < 0) {
    phi[index] = 0.0;
    effective_phi[index] = 0.0 - radii[index];
    colliding[index] = 0;
    block_ids[index] = -1;
    fallback_needed[index] = 1;
    return;
  }
  double value = 0.0;
  const bool sampled =
      sampleBlockDevice(metadata[block_index], values, point, &value);
  if (!sampled) {
    phi[index] = 0.0;
    effective_phi[index] = 0.0 - radii[index];
    colliding[index] = 0;
    block_ids[index] = metadata[block_index].block_id;
    fallback_needed[index] = 1;
    return;
  }
  phi[index] = value;
  effective_phi[index] = value - radii[index];
  colliding[index] = effective_phi[index] <= threshold ? 1 : 0;
  block_ids[index] = metadata[block_index].block_id;
  fallback_needed[index] = 0;
}

__global__ void cudaActiveBlockQueryPhiNormalKernel(
    const CudaActiveBlockMetadata* metadata,
    int block_count,
    const double* values,
    const DeviceVec3* points,
    const double* radii,
    std::size_t sample_count,
    double threshold,
    double* phi,
    double* effective_phi,
    int* colliding,
    int* block_ids,
    int* fallback_needed,
    DeviceVec3* normals) {
  const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= sample_count) {
    return;
  }
  const DeviceVec3 point = points[index];
  const int block_index = chooseBlockDevice(metadata, block_count, point);
  if (block_index < 0) {
    phi[index] = 0.0;
    effective_phi[index] = 0.0 - radii[index];
    colliding[index] = 0;
    block_ids[index] = -1;
    fallback_needed[index] = 1;
    normals[index] = {0.0, 0.0, 0.0};
    return;
  }
  double value = 0.0;
  const bool sampled =
      sampleBlockDevice(metadata[block_index], values, point, &value);
  if (!sampled) {
    phi[index] = 0.0;
    effective_phi[index] = 0.0 - radii[index];
    colliding[index] = 0;
    block_ids[index] = metadata[block_index].block_id;
    fallback_needed[index] = 1;
    normals[index] = {0.0, 0.0, 0.0};
    return;
  }
  phi[index] = value;
  effective_phi[index] = value - radii[index];
  colliding[index] = effective_phi[index] <= threshold ? 1 : 0;
  block_ids[index] = metadata[block_index].block_id;
  fallback_needed[index] = 0;
  normals[index] = normalFromBlockDevice(metadata[block_index], values, point);
}

}  // namespace

void* createActiveBlockBufferOnCuda() {
  return new ActiveBlockBufferHandle();
}

bool uploadActiveBlockBufferOnCuda(
    void* handle,
    const std::vector<CudaActiveBlockMetadata>& metadata,
    const std::vector<double>& values,
    std::size_t* device_memory_bytes,
    double* upload_time_ms,
    std::string* error_message) {
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(handle);
  if (buffer == nullptr || metadata.empty() || values.empty()) {
    if (error_message != nullptr) {
      *error_message = "invalid CUDA active block upload input.";
    }
    return false;
  }
  releaseHandle(buffer);
  const auto start = Clock::now();
  const std::size_t metadata_bytes =
      metadata.size() * sizeof(CudaActiveBlockMetadata);
  const std::size_t value_bytes = values.size() * sizeof(double);
  if (!checkCuda(
          cudaMalloc(&buffer->device_metadata, metadata_bytes),
          error_message)) {
    releaseHandle(buffer);
    return false;
  }
  if (!checkCuda(cudaMalloc(&buffer->device_values, value_bytes), error_message)) {
    releaseHandle(buffer);
    return false;
  }
  if (!checkCuda(
          cudaMemcpy(
              buffer->device_metadata,
              metadata.data(),
              metadata_bytes,
              cudaMemcpyHostToDevice),
          error_message)) {
    releaseHandle(buffer);
    return false;
  }
  if (!checkCuda(
          cudaMemcpy(
              buffer->device_values,
              values.data(),
              value_bytes,
              cudaMemcpyHostToDevice),
          error_message)) {
    releaseHandle(buffer);
    return false;
  }
  if (!checkCuda(cudaDeviceSynchronize(), error_message)) {
    releaseHandle(buffer);
    return false;
  }
  buffer->block_count = static_cast<int>(metadata.size());
  buffer->value_count = static_cast<int>(values.size());
  buffer->memory_bytes = metadata_bytes + value_bytes;
  if (device_memory_bytes != nullptr) {
    *device_memory_bytes = buffer->memory_bytes;
  }
  if (upload_time_ms != nullptr) {
    *upload_time_ms = elapsedMs(start, Clock::now());
  }
  return true;
}

void releaseActiveBlockBufferOnCuda(void* handle) {
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(handle);
  releaseHandle(buffer);
  delete buffer;
}

std::size_t activeBlockBufferBlockCountOnCuda(void* handle) {
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(handle);
  return buffer == nullptr ? 0 : static_cast<std::size_t>(buffer->block_count);
}

std::size_t activeBlockBufferValueCountOnCuda(void* handle) {
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(handle);
  return buffer == nullptr ? 0 : static_cast<std::size_t>(buffer->value_count);
}

std::size_t activeBlockBufferDeviceMemoryBytesOnCuda(void* handle) {
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(handle);
  return buffer == nullptr ? 0 : buffer->memory_bytes;
}

bool queryActiveBlockBufferOnCuda(
    void* buffer_handle,
    const std::vector<Vector3>& points,
    const std::vector<double>& radii,
    double threshold,
    bool compute_normals,
    CudaActiveBlockDeviceQueryResult* output) {
  if (output == nullptr) {
    return false;
  }
  *output = {};
  auto* buffer = static_cast<ActiveBlockBufferHandle*>(buffer_handle);
  if (buffer == nullptr || buffer->device_metadata == nullptr ||
      buffer->device_values == nullptr) {
    output->error_message = "CUDA active block query requires an uploaded buffer.";
    return false;
  }
  if (points.size() != radii.size()) {
    output->error_message = "CUDA active block query points/radii size mismatch.";
    return false;
  }
  const std::size_t count = points.size();
  if (count == 0) {
    output->success = true;
    return true;
  }

  DeviceVec3* device_points = nullptr;
  double* device_radii = nullptr;
  double* device_phi = nullptr;
  double* device_effective_phi = nullptr;
  int* device_colliding = nullptr;
  int* device_block_ids = nullptr;
  int* device_fallback_needed = nullptr;
  DeviceVec3* device_normals = nullptr;
  cudaEvent_t kernel_start = nullptr;
  cudaEvent_t kernel_stop = nullptr;

  auto cleanup = [&]() {
    if (device_points != nullptr) cudaFree(device_points);
    if (device_radii != nullptr) cudaFree(device_radii);
    if (device_phi != nullptr) cudaFree(device_phi);
    if (device_effective_phi != nullptr) cudaFree(device_effective_phi);
    if (device_colliding != nullptr) cudaFree(device_colliding);
    if (device_block_ids != nullptr) cudaFree(device_block_ids);
    if (device_fallback_needed != nullptr) cudaFree(device_fallback_needed);
    if (device_normals != nullptr) cudaFree(device_normals);
    if (kernel_start != nullptr) cudaEventDestroy(kernel_start);
    if (kernel_stop != nullptr) cudaEventDestroy(kernel_stop);
  };

  std::string error;
  const auto upload_start = Clock::now();
  std::vector<DeviceVec3> host_points;
  host_points.reserve(count);
  for (const Vector3& point : points) {
    host_points.push_back({point.x, point.y, point.z});
  }
  if (!checkCuda(cudaMalloc(&device_points, count * sizeof(DeviceVec3)), &error) ||
      !checkCuda(cudaMalloc(&device_radii, count * sizeof(double)), &error) ||
      !checkCuda(cudaMalloc(&device_phi, count * sizeof(double)), &error) ||
      !checkCuda(cudaMalloc(&device_effective_phi, count * sizeof(double)), &error) ||
      !checkCuda(cudaMalloc(&device_colliding, count * sizeof(int)), &error) ||
      !checkCuda(cudaMalloc(&device_block_ids, count * sizeof(int)), &error) ||
      !checkCuda(cudaMalloc(&device_fallback_needed, count * sizeof(int)), &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }
  if (compute_normals &&
      !checkCuda(cudaMalloc(&device_normals, count * sizeof(DeviceVec3)), &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }
  if (!checkCuda(
          cudaMemcpy(
              device_points,
              host_points.data(),
              count * sizeof(DeviceVec3),
              cudaMemcpyHostToDevice),
          &error) ||
      !checkCuda(
          cudaMemcpy(
              device_radii,
              radii.data(),
              count * sizeof(double),
              cudaMemcpyHostToDevice),
          &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }
  output->sample_upload_time_ms = elapsedMs(upload_start, Clock::now());

  if (!checkCuda(cudaEventCreate(&kernel_start), &error) ||
      !checkCuda(cudaEventCreate(&kernel_stop), &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }

  const int threads = 256;
  const int blocks = static_cast<int>((count + threads - 1) / threads);
  cudaEventRecord(kernel_start);
  if (compute_normals) {
    cudaActiveBlockQueryPhiNormalKernel<<<blocks, threads>>>(
        buffer->device_metadata,
        buffer->block_count,
        buffer->device_values,
        device_points,
        device_radii,
        count,
        threshold,
        device_phi,
        device_effective_phi,
        device_colliding,
        device_block_ids,
        device_fallback_needed,
        device_normals);
  } else {
    cudaActiveBlockQueryPhiKernel<<<blocks, threads>>>(
        buffer->device_metadata,
        buffer->block_count,
        buffer->device_values,
        device_points,
        device_radii,
        count,
        threshold,
        device_phi,
        device_effective_phi,
        device_colliding,
        device_block_ids,
        device_fallback_needed);
  }
  cudaEventRecord(kernel_stop);
  if (!checkCuda(cudaPeekAtLastError(), &error) ||
      !checkCuda(cudaEventSynchronize(kernel_stop), &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }
  float kernel_ms = 0.0f;
  cudaEventElapsedTime(&kernel_ms, kernel_start, kernel_stop);
  output->kernel_time_ms = static_cast<double>(kernel_ms);

  const auto download_start = Clock::now();
  output->phi.resize(count);
  output->effective_phi.resize(count);
  output->colliding.resize(count);
  output->block_ids.resize(count);
  output->fallback_needed.resize(count);
  if (!checkCuda(
          cudaMemcpy(
              output->phi.data(),
              device_phi,
              count * sizeof(double),
              cudaMemcpyDeviceToHost),
          &error) ||
      !checkCuda(
          cudaMemcpy(
              output->effective_phi.data(),
              device_effective_phi,
              count * sizeof(double),
              cudaMemcpyDeviceToHost),
          &error) ||
      !checkCuda(
          cudaMemcpy(
              output->colliding.data(),
              device_colliding,
              count * sizeof(int),
              cudaMemcpyDeviceToHost),
          &error) ||
      !checkCuda(
          cudaMemcpy(
              output->block_ids.data(),
              device_block_ids,
              count * sizeof(int),
              cudaMemcpyDeviceToHost),
          &error) ||
      !checkCuda(
          cudaMemcpy(
              output->fallback_needed.data(),
              device_fallback_needed,
              count * sizeof(int),
              cudaMemcpyDeviceToHost),
          &error)) {
    output->error_message = error;
    cleanup();
    return false;
  }
  if (compute_normals) {
    std::vector<DeviceVec3> host_normals(count);
    if (!checkCuda(
            cudaMemcpy(
                host_normals.data(),
                device_normals,
                count * sizeof(DeviceVec3),
                cudaMemcpyDeviceToHost),
            &error)) {
      output->error_message = error;
      cleanup();
      return false;
    }
    output->normals.reserve(count);
    for (const DeviceVec3& n : host_normals) {
      output->normals.push_back({n.x, n.y, n.z});
    }
  }
  output->download_time_ms = elapsedMs(download_start, Clock::now());
  output->success = true;
  cleanup();
  return true;
}

}  // namespace cuda_detail
}  // namespace adasdf
