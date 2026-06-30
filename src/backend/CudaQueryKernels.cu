#include "adasdf/backend/CudaQueryBackend.h"

#include <cuda_runtime.h>

#include <algorithm>
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

struct CudaQueryWorkspaceHandle {
  DeviceVec3* device_points = nullptr;
  double* device_phi = nullptr;
  DeviceVec3* device_normals = nullptr;
  std::size_t capacity = 0;
  std::size_t memory_bytes = 0;
  std::size_t allocation_count = 0;
  bool last_ensure_reused = false;
  bool need_normals = false;
};

using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

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
  if (block_count == 1) {
    if (sampleExpandedBlock(blocks[0], values, p, phi)) {
      if (used_block != nullptr) {
        *used_block = &blocks[0];
      }
      return true;
    }
    return false;
  }
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

__device__ bool sampleExpandedSDFPreferBlock(
    const DeviceExpandedBlock* blocks,
    int block_count,
    const double* values,
    const DeviceVec3& p,
    const DeviceExpandedBlock* preferred_block,
    double* phi) {
  if (preferred_block != nullptr &&
      sampleExpandedBlock(*preferred_block, values, p, phi)) {
    return true;
  }
  if (block_count == 1) {
    return false;
  }
  for (int i = 0; i < block_count; ++i) {
    if (preferred_block != nullptr && &blocks[i] == preferred_block) {
      continue;
    }
    if (sampleExpandedBlock(blocks[i], values, p, phi)) {
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
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x + h, p.y, p.z}, used_block, &px);
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x - h, p.y, p.z}, used_block, &mx);
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x, p.y + h, p.z}, used_block, &py);
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x, p.y - h, p.z}, used_block, &my);
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x, p.y, p.z + h}, used_block, &pz);
  ok = ok && sampleExpandedSDFPreferBlock(
                 blocks, block_count, values, {p.x, p.y, p.z - h}, used_block, &mz);
  if (!ok) {
    normals[index] = {1.0, 0.0, 0.0};
    return;
  }

  normals[index] = normalizedOrFallbackDevice(
      {(px - mx) / (2.0 * h),
       (py - my) / (2.0 * h),
       (pz - mz) / (2.0 * h)});
}

__global__ void expandedSdfPhiOnlyKernel(
    const DeviceVec3* points,
    std::size_t count,
    const DeviceExpandedBlock* blocks,
    int block_count,
    const double* values,
    double* phi) {
  const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }

  double value = 1.0e30;
  sampleExpandedSDF(
      blocks,
      block_count,
      values,
      points[index],
      &value,
      nullptr);
  phi[index] = value;
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

void freeWorkspaceBuffers(CudaQueryWorkspaceHandle* workspace) {
  if (workspace == nullptr) {
    return;
  }
  cudaFree(workspace->device_points);
  cudaFree(workspace->device_phi);
  cudaFree(workspace->device_normals);
  workspace->device_points = nullptr;
  workspace->device_phi = nullptr;
  workspace->device_normals = nullptr;
  workspace->capacity = 0;
  workspace->memory_bytes = 0;
  workspace->last_ensure_reused = false;
  workspace->need_normals = false;
}

bool ensureWorkspaceCapacity(
    CudaQueryWorkspaceHandle* workspace,
    std::size_t capacity,
    bool need_normals,
    BatchQueryTiming* timing) {
  if (workspace == nullptr) {
    return false;
  }
  if (capacity == 0) {
    workspace->last_ensure_reused = true;
    return true;
  }
  if (workspace->capacity >= capacity &&
      (!need_normals || workspace->device_normals != nullptr)) {
    workspace->need_normals = need_normals;
    workspace->last_ensure_reused = true;
    return true;
  }

  const auto t0 = Clock::now();
  freeWorkspaceBuffers(workspace);
  const std::size_t point_bytes = capacity * sizeof(DeviceVec3);
  const std::size_t phi_bytes = capacity * sizeof(double);
  try {
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&workspace->device_points), point_bytes),
        "cudaMalloc(query workspace points)");
    checkCuda(
        cudaMalloc(reinterpret_cast<void**>(&workspace->device_phi), phi_bytes),
        "cudaMalloc(query workspace phi)");
    if (need_normals) {
      checkCuda(
          cudaMalloc(
              reinterpret_cast<void**>(&workspace->device_normals),
              point_bytes),
          "cudaMalloc(query workspace normals)");
    }
  } catch (...) {
    freeWorkspaceBuffers(workspace);
    throw;
  }
  workspace->capacity = capacity;
  workspace->need_normals = need_normals;
  workspace->memory_bytes = point_bytes + phi_bytes + (need_normals ? point_bytes : 0);
  workspace->allocation_count += 1;
  workspace->last_ensure_reused = false;
  if (timing != nullptr) {
    timing->allocation_ms += elapsedMs(t0, Clock::now());
  }
  return true;
}

std::vector<DeviceVec3> packPoints(const std::vector<Vector3>& points) {
  std::vector<DeviceVec3> host_points;
  host_points.reserve(points.size());
  for (const Vector3& point : points) {
    host_points.push_back(toDeviceVec3(point));
  }
  return host_points;
}

void uploadWorkspacePoints(
    CudaQueryWorkspaceHandle* workspace,
    const std::vector<Vector3>& points,
    BatchQueryTiming* timing) {
  if (workspace == nullptr || workspace->device_points == nullptr ||
      points.size() > workspace->capacity) {
    throw std::runtime_error("CUDA query workspace is not large enough for points.");
  }
  const auto t0 = Clock::now();
  const std::vector<DeviceVec3> host_points = packPoints(points);
  if (!host_points.empty()) {
    checkCuda(
        cudaMemcpy(
            workspace->device_points,
            host_points.data(),
            host_points.size() * sizeof(DeviceVec3),
            cudaMemcpyHostToDevice),
        "cudaMemcpy(query workspace points)");
  }
  if (timing != nullptr) {
    timing->h2d_points_ms += elapsedMs(t0, Clock::now());
  }
}

void downloadWorkspaceResultsInto(
    CudaQueryWorkspaceHandle* workspace,
    std::size_t count,
    bool need_normals,
    BatchQueryOutput* output,
    BatchQueryTiming* timing) {
  if (workspace == nullptr || workspace->device_phi == nullptr ||
      count > workspace->capacity) {
    throw std::runtime_error("CUDA query workspace is not ready for download.");
  }
  if (output == nullptr) {
    throw std::runtime_error("CUDA query download requires output storage.");
  }

  const auto allocation0 = Clock::now();
  output->signed_distances.resize(count);
  if (need_normals) {
    output->gradients.resize(count);
    output->normals.resize(count);
  } else {
    output->gradients.clear();
    output->normals.clear();
  }
  const auto allocation1 = Clock::now();
  if (timing != nullptr) {
    timing->allocation_ms += elapsedMs(allocation0, allocation1);
  }

  if (count == 0) {
    return;
  }

  const auto d2h0 = Clock::now();
  checkCuda(
      cudaMemcpy(
          output->signed_distances.data(),
          workspace->device_phi,
          count * sizeof(double),
          cudaMemcpyDeviceToHost),
      "cudaMemcpy(query workspace phi)");

  std::vector<DeviceVec3> host_normals;
  if (need_normals) {
    if (workspace->device_normals == nullptr) {
      throw std::runtime_error("CUDA query workspace normals were not allocated.");
    }
    host_normals.resize(count);
    checkCuda(
        cudaMemcpy(
            host_normals.data(),
            workspace->device_normals,
            count * sizeof(DeviceVec3),
            cudaMemcpyDeviceToHost),
        "cudaMemcpy(query workspace normals)");
  }
  const auto d2h1 = Clock::now();
  if (timing != nullptr) {
    timing->d2h_results_ms += elapsedMs(d2h0, d2h1);
  }

  if (need_normals) {
    const auto post0 = Clock::now();
    for (std::size_t i = 0; i < host_normals.size(); ++i) {
      output->gradients[i] = toVector3(host_normals[i]);
      output->normals[i] = output->gradients[i];
    }
    if (timing != nullptr) {
      timing->postprocess_ms += elapsedMs(post0, Clock::now());
    }
  }
}

BatchQueryOutput downloadWorkspaceResults(
    CudaQueryWorkspaceHandle* workspace,
    std::size_t count,
    bool need_normals,
    BatchQueryTiming* timing) {
  BatchQueryOutput output;
  downloadWorkspaceResultsInto(workspace, count, need_normals, &output, timing);
  return output;
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

void* createQueryWorkspaceOnCuda() {
  return new CudaQueryWorkspaceHandle();
}

bool ensureQueryWorkspaceOnCuda(
    void* workspace_handle,
    std::size_t capacity,
    bool need_normals,
    std::size_t* device_memory_bytes) {
  if (workspace_handle == nullptr) {
    return false;
  }
  auto* workspace = static_cast<CudaQueryWorkspaceHandle*>(workspace_handle);
  ensureWorkspaceCapacity(workspace, capacity, need_normals, nullptr);
  if (device_memory_bytes != nullptr) {
    *device_memory_bytes = workspace->memory_bytes;
  }
  return true;
}

bool uploadQueryWorkspacePointsOnCuda(
    void* workspace_handle,
    const std::vector<Vector3>& points,
    BatchQueryTiming* timing) {
  if (workspace_handle == nullptr) {
    return false;
  }
  auto* workspace = static_cast<CudaQueryWorkspaceHandle*>(workspace_handle);
  uploadWorkspacePoints(workspace, points, timing);
  return true;
}

BatchQueryOutput downloadQueryWorkspaceResultsOnCuda(
    void* workspace_handle,
    std::size_t count,
    bool need_normals,
    BatchQueryTiming* timing) {
  if (workspace_handle == nullptr) {
    throw std::runtime_error("downloadQueryWorkspaceResultsOnCuda received null workspace.");
  }
  auto* workspace = static_cast<CudaQueryWorkspaceHandle*>(workspace_handle);
  return downloadWorkspaceResults(workspace, count, need_normals, timing);
}

void releaseQueryWorkspaceOnCuda(void* workspace_handle) {
  if (workspace_handle == nullptr) {
    return;
  }
  auto* workspace = static_cast<CudaQueryWorkspaceHandle*>(workspace_handle);
  freeWorkspaceBuffers(workspace);
  delete workspace;
}

std::size_t queryWorkspaceCapacityOnCuda(void* workspace_handle) {
  if (workspace_handle == nullptr) {
    return 0;
  }
  return static_cast<CudaQueryWorkspaceHandle*>(workspace_handle)->capacity;
}

std::size_t queryWorkspaceDeviceMemoryBytesOnCuda(void* workspace_handle) {
  if (workspace_handle == nullptr) {
    return 0;
  }
  return static_cast<CudaQueryWorkspaceHandle*>(workspace_handle)->memory_bytes;
}

std::size_t queryWorkspaceAllocationCountOnCuda(void* workspace_handle) {
  if (workspace_handle == nullptr) {
    return 0;
  }
  return static_cast<CudaQueryWorkspaceHandle*>(workspace_handle)->allocation_count;
}

bool queryWorkspaceLastEnsureReusedOnCuda(void* workspace_handle) {
  if (workspace_handle == nullptr) {
    return false;
  }
  return static_cast<CudaQueryWorkspaceHandle*>(workspace_handle)->last_ensure_reused;
}

bool queryExpandedSDFOnCuda(
    void* device_handle,
    const std::vector<Vector3>& points,
    QueryOutputMode output_mode,
    bool download_results,
    void* workspace_handle,
    BatchQueryOutput* output,
    BatchQueryTiming* timing) {
  if (device_handle == nullptr) {
    throw std::runtime_error("queryExpandedSDFOnCuda received null resident handle.");
  }
  auto* handle = static_cast<ResidentExpandedSDFHandle*>(device_handle);

  BatchQueryTiming local_timing;
  const auto total0 = Clock::now();
  const bool need_normals = includesNormals(output_mode);
  local_timing.download_results = download_results;
  local_timing.correctness_checked = download_results;
  if (points.empty()) {
    if (output != nullptr) {
      output->signed_distances.clear();
      output->gradients.clear();
      output->normals.clear();
    }
    if (timing != nullptr) {
      local_timing.total_ms = elapsedMs(total0, Clock::now());
      *timing = local_timing;
    }
    return true;
  }

  CudaQueryWorkspaceHandle* workspace =
      static_cast<CudaQueryWorkspaceHandle*>(workspace_handle);
  bool temporary_workspace = false;
  cudaEvent_t start{};
  cudaEvent_t stop{};

  try {
    if (workspace == nullptr) {
      workspace = new CudaQueryWorkspaceHandle();
      temporary_workspace = true;
    }
    ensureWorkspaceCapacity(workspace, points.size(), need_normals, &local_timing);
    uploadWorkspacePoints(workspace, points, &local_timing);

    checkCuda(cudaEventCreate(&start), "cudaEventCreate(start)");
    checkCuda(cudaEventCreate(&stop), "cudaEventCreate(stop)");
    constexpr int block_size = 256;
    const int grid_size =
        static_cast<int>((points.size() + block_size - 1) / block_size);
    checkCuda(cudaEventRecord(start), "cudaEventRecord(start)");
    if (output_mode == QueryOutputMode::PhiOnly) {
      expandedSdfPhiOnlyKernel<<<grid_size, block_size>>>(
          workspace->device_points,
          points.size(),
          handle->device_blocks,
          handle->block_count,
          handle->device_values,
          workspace->device_phi);
      checkCuda(cudaGetLastError(), "expandedSdfPhiOnlyKernel launch");
    } else {
      expandedSdfKernel<<<grid_size, block_size>>>(
          workspace->device_points,
          points.size(),
          handle->device_blocks,
          handle->block_count,
          handle->device_values,
          workspace->device_phi,
          workspace->device_normals);
      checkCuda(cudaGetLastError(), "expandedSdfKernel launch");
    }
    const auto sync0 = Clock::now();
    checkCuda(cudaEventRecord(stop), "cudaEventRecord(stop)");
    checkCuda(cudaEventSynchronize(stop), "cudaEventSynchronize(stop)");
    const auto sync1 = Clock::now();
    float kernel_ms = 0.0f;
    checkCuda(cudaEventElapsedTime(&kernel_ms, start, stop), "cudaEventElapsedTime");
    local_timing.kernel_ms = static_cast<double>(kernel_ms);
    local_timing.sync_ms = elapsedMs(sync0, sync1);
    local_timing.workspace_reused = workspace->last_ensure_reused;
    local_timing.allocation_count = workspace->allocation_count;
    local_timing.workspace_capacity = workspace->capacity;
    local_timing.workspace_device_memory_mb =
        static_cast<double>(workspace->memory_bytes) / (1024.0 * 1024.0);
    if (handle->block_count == 1) {
      local_timing.block_lookup_count = points.size();
      local_timing.block_scan_count = 0;
      local_timing.center_block_hit_rate = 1.0;
      local_timing.neighbor_same_block_rate =
          output_mode == QueryOutputMode::PhiOnly ? 1.0 : 1.0;
    } else {
      const std::size_t samples_per_point =
          output_mode == QueryOutputMode::PhiOnly ? 1 : 7;
      local_timing.block_lookup_count = points.size() * samples_per_point;
      local_timing.block_scan_count = points.size();
      local_timing.center_block_hit_rate = 0.0;
      local_timing.neighbor_same_block_rate = 0.0;
    }

    if (download_results) {
      downloadWorkspaceResultsInto(
          workspace,
          points.size(),
          need_normals,
          output,
          &local_timing);
    } else if (output != nullptr) {
      output->signed_distances.clear();
      output->gradients.clear();
      output->normals.clear();
    }
    const auto free0 = Clock::now();
    if (temporary_workspace) {
      freeWorkspaceBuffers(workspace);
      delete workspace;
      workspace = nullptr;
    }
    local_timing.free_ms += elapsedMs(free0, Clock::now());
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    local_timing.total_ms = elapsedMs(total0, Clock::now());
    if (timing != nullptr) {
      *timing = local_timing;
    }
    return true;
  } catch (...) {
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    if (temporary_workspace) {
      freeWorkspaceBuffers(workspace);
      delete workspace;
    }
    throw;
  }
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
