#include "adasdf/backend/CudaQueryBackend.h"

#include <cuda_runtime.h>

#include <algorithm>
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

struct DeviceExpandedBlock {
  int block_id;
  DeviceVec3 min_corner;
  DeviceVec3 max_corner;
  int resolution_x;
  int resolution_y;
  int resolution_z;
  unsigned long long value_offset;
};

struct ResidentExpandedSDFHandle {
  ExpandedSDFLayout layout = ExpandedSDFLayout::GlobalDense;
  DeviceExpandedBlock* device_blocks = nullptr;
  double* device_values = nullptr;
  int block_count = 0;
  unsigned long long value_count = 0;
  std::size_t memory_bytes = 0;
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

__device__ unsigned long long expandedValueIndex(
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

__device__ bool deviceContains(
    const DeviceExpandedBlock& block,
    const DeviceVec3& p) {
  const double eps = 1.0e-12;
  return p.x >= block.min_corner.x - eps && p.x <= block.max_corner.x + eps &&
         p.y >= block.min_corner.y - eps && p.y <= block.max_corner.y + eps &&
         p.z >= block.min_corner.z - eps && p.z <= block.max_corner.z + eps;
}

__device__ bool sampleExpandedBlock(
    const DeviceExpandedBlock& block,
    const double* values,
    const DeviceVec3& p,
    double* phi) {
  if (!deviceContains(block, p) || block.resolution_x < 2 ||
      block.resolution_y < 2 || block.resolution_z < 2) {
    return false;
  }

  const double sx = (p.x - block.min_corner.x) /
                    (block.max_corner.x - block.min_corner.x) *
                    static_cast<double>(block.resolution_x - 1);
  const double sy = (p.y - block.min_corner.y) /
                    (block.max_corner.y - block.min_corner.y) *
                    static_cast<double>(block.resolution_y - 1);
  const double sz = (p.z - block.min_corner.z) /
                    (block.max_corner.z - block.min_corner.z) *
                    static_cast<double>(block.resolution_z - 1);

  int i = static_cast<int>(floor(sx));
  int j = static_cast<int>(floor(sy));
  int k = static_cast<int>(floor(sz));
  i = max(0, min(i, block.resolution_x - 2));
  j = max(0, min(j, block.resolution_y - 2));
  k = max(0, min(k, block.resolution_z - 2));

  const double ax = fmin(1.0, fmax(0.0, sx - static_cast<double>(i)));
  const double ay = fmin(1.0, fmax(0.0, sy - static_cast<double>(j)));
  const double az = fmin(1.0, fmax(0.0, sz - static_cast<double>(k)));
  const unsigned long long base = block.value_offset;
  const int nx = block.resolution_x;
  const int ny = block.resolution_y;

  const double c000 = values[base + expandedValueIndex(i, j, k, nx, ny)];
  const double c100 = values[base + expandedValueIndex(i + 1, j, k, nx, ny)];
  const double c010 = values[base + expandedValueIndex(i, j + 1, k, nx, ny)];
  const double c110 = values[base + expandedValueIndex(i + 1, j + 1, k, nx, ny)];
  const double c001 = values[base + expandedValueIndex(i, j, k + 1, nx, ny)];
  const double c101 = values[base + expandedValueIndex(i + 1, j, k + 1, nx, ny)];
  const double c011 = values[base + expandedValueIndex(i, j + 1, k + 1, nx, ny)];
  const double c111 = values[base + expandedValueIndex(i + 1, j + 1, k + 1, nx, ny)];

  const double c00 = (1.0 - ax) * c000 + ax * c100;
  const double c10 = (1.0 - ax) * c010 + ax * c110;
  const double c01 = (1.0 - ax) * c001 + ax * c101;
  const double c11 = (1.0 - ax) * c011 + ax * c111;
  const double c0 = (1.0 - ay) * c00 + ay * c10;
  const double c1 = (1.0 - ay) * c01 + ay * c11;
  *phi = (1.0 - az) * c0 + az * c1;
  return true;
}

__device__ bool sampleExpandedSDF(
    const DeviceExpandedBlock* blocks,
    int block_count,
    const double* values,
    const DeviceVec3& p,
    double* phi,
    const DeviceExpandedBlock** used_block) {
  for (int i = 0; i < block_count; ++i) {
    if (sampleExpandedBlock(blocks[i], values, p, phi)) {
      if (used_block != nullptr) {
        *used_block = &blocks[i];
      }
      return true;
    }
  }
  return false;
}

__device__ double gradientStep(const DeviceExpandedBlock& block) {
  const double hx = (block.max_corner.x - block.min_corner.x) /
                    static_cast<double>(max(1, block.resolution_x - 1));
  const double hy = (block.max_corner.y - block.min_corner.y) /
                    static_cast<double>(max(1, block.resolution_y - 1));
  const double hz = (block.max_corner.z - block.min_corner.z) /
                    static_cast<double>(max(1, block.resolution_z - 1));
  return 0.5 * fmin(hx, fmin(hy, hz));
}

__global__ void expandedSdfKernel(
    const DeviceVec3* points,
    std::size_t count,
    const DeviceExpandedBlock* blocks,
    int block_count,
    const double* values,
    double* phi,
    DeviceVec3* normals) {
  const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }

  const DeviceVec3 p = points[index];
  const DeviceExpandedBlock* used_block = nullptr;
  double value = 1.0e30;
  if (!sampleExpandedSDF(blocks, block_count, values, p, &value, &used_block) ||
      used_block == nullptr) {
    phi[index] = value;
    normals[index] = {1.0, 0.0, 0.0};
    return;
  }
  phi[index] = value;

  const double h = gradientStep(*used_block);
  if (!(h > 0.0)) {
    normals[index] = {1.0, 0.0, 0.0};
    return;
  }

  double px = value;
  double mx = value;
  double py = value;
  double my = value;
  double pz = value;
  double mz = value;
  bool ok = true;
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x + h, p.y, p.z}, &px, nullptr);
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x - h, p.y, p.z}, &mx, nullptr);
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x, p.y + h, p.z}, &py, nullptr);
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x, p.y - h, p.z}, &my, nullptr);
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x, p.y, p.z + h}, &pz, nullptr);
  ok = ok && sampleExpandedSDF(
                 blocks, block_count, values, {p.x, p.y, p.z - h}, &mz, nullptr);
  if (!ok) {
    normals[index] = {1.0, 0.0, 0.0};
    return;
  }

  normals[index] = normalizedOrFallbackDevice(
      {(px - mx) / (2.0 * h),
       (py - my) / (2.0 * h),
       (pz - mz) / (2.0 * h)});
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

void* uploadExpandedSDFToCuda(
    const ExpandedSDF& expanded,
    std::size_t* device_memory_bytes) {
  if (!expanded.isValid() || expanded.blocks().empty()) {
    throw std::runtime_error("uploadExpandedSDFToCuda requires valid expanded data.");
  }

  std::vector<DeviceExpandedBlock> host_blocks;
  std::vector<double> host_values;
  host_blocks.reserve(expanded.blocks().size());
  for (const ExpandedBlock& block : expanded.blocks()) {
    DeviceExpandedBlock device_block;
    device_block.block_id = block.block_id;
    device_block.min_corner = toDeviceVec3(block.min_corner);
    device_block.max_corner = toDeviceVec3(block.max_corner);
    device_block.resolution_x = block.resolution_x;
    device_block.resolution_y = block.resolution_y;
    device_block.resolution_z = block.resolution_z;
    device_block.value_offset =
        static_cast<unsigned long long>(host_values.size());
    host_values.insert(host_values.end(), block.values.begin(), block.values.end());
    host_blocks.push_back(device_block);
  }

  auto* handle = new ResidentExpandedSDFHandle();
  handle->layout = expanded.layout();
  handle->block_count = static_cast<int>(host_blocks.size());
  handle->value_count = static_cast<unsigned long long>(host_values.size());

  const std::size_t block_bytes =
      host_blocks.size() * sizeof(DeviceExpandedBlock);
  const std::size_t value_bytes = host_values.size() * sizeof(double);

  try {
    checkCuda(
        cudaMalloc(
            reinterpret_cast<void**>(&handle->device_blocks),
            block_bytes),
        "cudaMalloc(expanded blocks)");
    checkCuda(
        cudaMalloc(
            reinterpret_cast<void**>(&handle->device_values),
            value_bytes),
        "cudaMalloc(expanded values)");
    checkCuda(
        cudaMemcpy(
            handle->device_blocks,
            host_blocks.data(),
            block_bytes,
            cudaMemcpyHostToDevice),
        "cudaMemcpy(expanded blocks)");
    checkCuda(
        cudaMemcpy(
            handle->device_values,
            host_values.data(),
            value_bytes,
            cudaMemcpyHostToDevice),
        "cudaMemcpy(expanded values)");
    checkCuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize(upload expanded)");
  } catch (...) {
    cudaFree(handle->device_blocks);
    cudaFree(handle->device_values);
    delete handle;
    throw;
  }

  handle->memory_bytes = block_bytes + value_bytes;
  if (device_memory_bytes != nullptr) {
    *device_memory_bytes = handle->memory_bytes;
  }
  return handle;
}

BatchQueryOutput queryExpandedSDFOnCuda(
    void* device_handle,
    const std::vector<Vector3>& points,
    double* kernel_ms_out) {
  if (device_handle == nullptr) {
    throw std::runtime_error("queryExpandedSDFOnCuda received null resident handle.");
  }
  auto* handle = static_cast<ResidentExpandedSDFHandle*>(device_handle);

  BatchQueryOutput output;
  output.signed_distances.resize(points.size());
  output.gradients.resize(points.size());
  output.normals.resize(points.size());
  if (kernel_ms_out != nullptr) {
    *kernel_ms_out = 0.0;
  }
  if (points.empty()) {
    return output;
  }

  std::vector<DeviceVec3> host_points;
  host_points.reserve(points.size());
  for (const Vector3& point : points) {
    host_points.push_back(toDeviceVec3(point));
  }

  DeviceVec3* device_points = nullptr;
  double* device_phi = nullptr;
  DeviceVec3* device_normals = nullptr;
  cudaEvent_t start{};
  cudaEvent_t stop{};
  const std::size_t point_bytes = host_points.size() * sizeof(DeviceVec3);
  const std::size_t phi_bytes = points.size() * sizeof(double);

  try {
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_points), point_bytes),
        "cudaMalloc(expanded query points)");
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_phi), phi_bytes),
        "cudaMalloc(expanded query phi)");
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&device_normals), point_bytes),
        "cudaMalloc(expanded query normals)");
    checkCuda(
        cudaMemcpy(
            device_points,
            host_points.data(),
            point_bytes,
            cudaMemcpyHostToDevice),
        "cudaMemcpy(expanded query points)");

    checkCuda(cudaEventCreate(&start), "cudaEventCreate(start)");
    checkCuda(cudaEventCreate(&stop), "cudaEventCreate(stop)");
    constexpr int block_size = 256;
    const int grid_size =
        static_cast<int>((host_points.size() + block_size - 1) / block_size);
    checkCuda(cudaEventRecord(start), "cudaEventRecord(start)");
    expandedSdfKernel<<<grid_size, block_size>>>(
        device_points,
        host_points.size(),
        handle->device_blocks,
        handle->block_count,
        handle->device_values,
        device_phi,
        device_normals);
    checkCuda(cudaGetLastError(), "expandedSdfKernel launch");
    checkCuda(cudaEventRecord(stop), "cudaEventRecord(stop)");
    checkCuda(cudaEventSynchronize(stop), "cudaEventSynchronize(stop)");
    float kernel_ms = 0.0f;
    checkCuda(cudaEventElapsedTime(&kernel_ms, start, stop), "cudaEventElapsedTime");
    if (kernel_ms_out != nullptr) {
      *kernel_ms_out = static_cast<double>(kernel_ms);
    }

    std::vector<DeviceVec3> host_normals(points.size());
    checkCuda(
        cudaMemcpy(
            output.signed_distances.data(),
            device_phi,
            phi_bytes,
            cudaMemcpyDeviceToHost),
        "cudaMemcpy(expanded phi)");
    checkCuda(
        cudaMemcpy(
            host_normals.data(),
            device_normals,
            point_bytes,
            cudaMemcpyDeviceToHost),
        "cudaMemcpy(expanded normals)");
    for (std::size_t i = 0; i < host_normals.size(); ++i) {
      output.gradients[i] = toVector3(host_normals[i]);
      output.normals[i] = output.gradients[i];
    }
  } catch (...) {
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaFree(device_points);
    cudaFree(device_phi);
    cudaFree(device_normals);
    throw;
  }

  cudaEventDestroy(start);
  cudaEventDestroy(stop);
  cudaFree(device_points);
  cudaFree(device_phi);
  cudaFree(device_normals);
  return output;
}

void releaseExpandedSDFOnCuda(void* device_handle) {
  if (device_handle == nullptr) {
    return;
  }
  auto* handle = static_cast<ResidentExpandedSDFHandle*>(device_handle);
  cudaFree(handle->device_blocks);
  cudaFree(handle->device_values);
  delete handle;
}

}  // namespace cuda_detail
}  // namespace adasdf
