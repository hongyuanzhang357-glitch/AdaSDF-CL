# GPU Backend

CUDA support is a planned optional backend. The CPU library must remain usable without CUDA.

## Current Parent Project Assets

The existing project includes:

- `include/sdf/cuda/CudaGlobalDenseSDF.h`
- `include/sdf/cuda/CudaAllPointsSDFContact.h`
- `src/cuda/CudaGlobalDenseSDF.cu`
- `src/cuda/CudaAllPointsSDFContact.cu`
- CUDA tests and UI benchmark reports

These should be wrapped after the public CPU model and query API stabilizes.

## Public API Hooks

- `BackendType::CUDA`
- `GpuBackend`
- `BatchedCollisionRequest`
- `collideBatch(...)`
- `CollisionRequest::backend`
- `DistanceRequest::backend`

## Behavior Requirements

- CUDA should be opt-in at configure time.
- Missing CUDA should not prevent a CPU build.
- Runtime backend errors should be explicit.
- CPU/CUDA result differences should be tested with tolerances.
- Benchmark tools should record upload, kernel, download, reduction, and total times separately.

## TODO

- Define GPU memory ownership for `SDFModel::DeviceHandle`.
- Decide when `BackendType::Auto` chooses CUDA.
- Add batched contact output buffers with stable layout.
- Add tests comparing CUDA to CPU on representative `.sdfbin` models.
