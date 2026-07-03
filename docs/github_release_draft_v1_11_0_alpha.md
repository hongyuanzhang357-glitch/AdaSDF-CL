# AdaSDF-CL v1.11.0-alpha

AdaSDF-CL v1.11.0-alpha adds an optional CUDA active block cache baseline.

## Highlights

- Added `CudaActiveBlockBuffer`.
- Added `CudaActiveBlockCache`.
- Added `CudaActiveBlockQuery`.
- Added `CudaActiveBlockBenchmark`.
- Added `adasdf_cuda_active_block_query`.
- Added `adasdf_benchmark_cuda_block_cache`.
- Added Python wrappers for CUDA active block query and benchmarking.
- Added CPU-only unavailable-path coverage where CUDA tools return `20`.

## Boundary

v1.11 selects active blocks on CPU, expands active blocks on CPU, uploads the
expanded local dense blocks to GPU, and performs CUDA local dense-grid
interpolation.

It is not GPU-native compressed SVD reconstruction. It does not reconstruct
low-rank factors per point on GPU.

CUDA remains optional. CPU-only builds and install validation remain supported.

## Return Codes

- `adasdf_cuda_active_block_query` returns `10` when collision is detected.
- CUDA active block tools return `20` when CUDA is unavailable.

Return code `20` is a defined skip status, not a repository failure.
