# Original UI CUDA Audit

Date: 2026-06-29

This audit records how the original OctreeBlockLowRankSDF project implements
CUDA-backed UI and benchmark SDF queries, and which parts are directly useful
as a reference for AdaSDF-CL v1.0.1 CUDA validation.

## Scope

Audited files from the original project:

- `CMakeLists.txt`
- `include/sdf/cuda/CudaSDFCommon.cuh`
- `include/sdf/cuda/CudaGlobalDenseSDF.h`
- `include/sdf/cuda/CudaAllPointsSDFContact.h`
- `src/cuda/CudaGlobalDenseSDF.cu`
- `src/cuda/CudaAllPointsSDFContact.cu`
- `apps/sdf_tool.cpp`
- `apps/sdf_ui/backend/core_adapter.py`
- `apps/sdf_ui/widgets/gpu_benchmark_widget.py`
- Existing report CSVs under `reports/` and `paper_package/figures/`

No original core algorithm files were modified.

## Build Model

The original project keeps CUDA optional and disabled by default:

```text
SDF_ENABLE_CUDA=OFF by default
```

When `SDF_ENABLE_CUDA=ON`, the top-level CMake config:

- searches for `nvcc` in `CUDA_PATH/bin` and CUDA Toolkit install folders;
- fails configuration with a clear fatal error if `nvcc` is not found;
- sets `CMAKE_CUDA_COMPILER` to the discovered `nvcc`;
- defaults `CMAKE_CUDA_ARCHITECTURES` to `75` when not provided;
- enables the CUDA language;
- requires `CUDAToolkit`;
- adds `src/cuda/CudaGlobalDenseSDF.cu` and
  `src/cuda/CudaAllPointsSDFContact.cu` to `sdf_core`;
- links `CUDA::cudart`;
- builds CUDA-specific tests only when CUDA is enabled.

Therefore the original UI GPU benchmark needs a CUDA Toolkit and `nvcc`, not
only an NVIDIA driver.

## Runtime Query Kernels

`CudaGlobalDenseSDF` stores a full global dense SDF grid on the GPU:

- `CudaGridInfo` contains root bounds, fine-cell count, fine-node count, and
  grid spacing.
- Grid phi values are uploaded as `double`.
- Query points are packed as contiguous `double` triples.
- One CUDA thread evaluates one query point.
- Each point is transformed from source-local to world, then world to
  target-local, using row-major 3x4 transforms.
- Points outside the root grid are marked invalid and receive a large outside
  phi sentinel.
- Valid points are evaluated with trilinear interpolation on the dense grid.
- The kernel returns phi, valid flags, and inside flags.

`CudaBlockExpandedSDF` adds a second GPU storage layout:

- A micro-block map maps fine-grid locations to expanded dense blocks.
- Per-block metadata stores local grid offsets, dimensions, and value offsets.
- The query kernel performs the same transform and trilinear interpolation, but
  reads from the expanded block value buffer instead of one global dense array.

The original `compressed_factor_direct` GPU layout is explicitly unsupported in
the benchmark CLI. It reports:

```text
compressed_factor_direct GPU SDF factor query is not implemented; no dense fallback was used
```

## All-Points Contact CUDA Path

`CudaAllPointsSDFContact` is a prototype all-points contact evaluator:

- It samples mesh surface points with `SurfaceSampler`.
- It supports bidirectional A-to-B and B-to-A sampling.
- It builds `GlobalDenseGrid` objects from the adaptive models.
- It uploads each dense grid to `CudaGlobalDenseSDF`.
- It runs `CudaGlobalDenseSDF::queryPoints()` for each direction.
- CPU reference phi is computed on the host.
- CPU/GPU diff, sign mismatch count, inside count, min phi, and penetration
  summaries are reduced on the host.

The first version requires `useGlobalDense=true` and throws when asked to use a
non-global-dense contact mode.

## UI Call Chain

The Python UI does not call CUDA kernels directly. It shells out to the CUDA
build of `sdf_tool`:

- `gpu_benchmark_widget.py` runs `gpu-info` for backend availability.
- The GPU preflight runs `model-info`, `query-model`,
  `bench-gpu-sdf-query --help`, and then a small 100-point
  `bench-gpu-sdf-query`.
- The full GPU run starts `sdf_tool bench-gpu-sdf-query` through `QProcess`.
- The output CSV is written under the original project's `reports/` directory.
- The widget reads CSV rows and plots throughput and memory metrics.

The UI benchmark is therefore a CLI benchmark wrapper. The critical validation
surface is the CUDA `sdf_tool` executable and its CSV output, not PySide itself.

## Original CLI Benchmark Modes

`sdf_tool bench-gpu-sdf-query` accepts:

- `--model`
- `--stl`
- `--point-mode bbox|nearVertex|surfaceVertex`
- `--num`
- `--layout compressed_factor_direct|block_expanded|global_dense|all`
- `--warmup`
- `--repeat`
- `--cpu-reference auto|full|sample|off`
- `--out`

Its CSV includes:

- kernel min, mean, median, and standard deviation;
- ns/query and queries/second;
- setup, point generation, H2D, D2H, CSV write, and total wall time;
- compressed, dense-equivalent, block-expanded, and resident GPU memory;
- CPU reference error columns;
- sign mismatch count;
- support flags for phi, normal, and contact.

`sdf_tool bench-gpu-allpoints-sdf` accepts two model/STL pairs, calibrated cases,
sample mode, sample count, seed, and output path. Its CSV includes CPU and GPU
timings, inside counts, penetration summaries, and CPU/GPU diff metrics.

## Existing Benchmark Evidence

Representative original report rows found during the audit:

| Source report | Layout / case | Points | Kernel ms | Kernel ns/query | Total GPU ms | Status | Max abs diff | Sign mismatches |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| `paper_package/figures/ui_module_exports_v2/gpu_benchmark_all_10000_20260624_091716.csv` | `block_expanded` | 10,000 | 0.0191648 | 1.91648 | reported in shared wall columns | ok | 1.77636e-15 | 0 |
| `paper_package/figures/ui_module_exports_v2/gpu_benchmark_all_10000_20260624_091716.csv` | `global_dense` | 10,000 | 0.0239968 | 2.39968 | reported in shared wall columns | ok | 1.77636e-15 | 0 |
| `paper_package/figures/ui_module_exports_v2/gpu_benchmark_all_100000_20260624_091716.csv` | `block_expanded` | 100,000 | 0.127949 | 1.27949 | reported in shared wall columns | ok | 1.77636e-15 | 0 |
| `paper_package/figures/ui_module_exports_v2/gpu_benchmark_all_100000_20260624_091716.csv` | `global_dense` | 100,000 | 0.130432 | 1.30432 | reported in shared wall columns | ok | 1.77636e-15 | 0 |
| `reports/gpu_sdf_query_nearVertex_100000.csv` | `global_dense` | 100,000 | 0.487744 | 4.87744 | 3.2829 | ok | 1.77636e-15 | 0 |
| `reports/gpu_sdf_query_nearVertex_1000000.csv` | `global_dense` | 1,000,000 | 1.39347 | 1.39347 | 17.9906 | ok | 1.77636e-15 | 0 |
| `reports/gpu_allpoints_sdf_horse_1000000.csv` | `small-penetration` | 2,000,000 tested samples | 3.0088 | 1.5044 | 449.611 | ok | 1.42109e-14 | 0 |

There are also historical UI preflight rows where `global_dense` at 100 points
was ok, and one older 100k `gpu_test_global_dense_100k.csv` warning row with a
large CPU/GPU diff and sign mismatches. The later `ui_module_exports_v2` and
`gpu_sdf_query_nearVertex_*` reports show tight CPU/GPU agreement, so the audit
treats historical CSVs as evidence of multiple development stages rather than a
single immutable performance result.

## Takeaways For AdaSDF-CL

Useful patterns to carry forward:

- Keep CUDA optional.
- Make `nvcc` and CUDA Toolkit requirements explicit.
- Provide a CLI benchmark that can run outside the UI.
- Include CPU reference comparison and sign mismatch counts.
- Separate kernel time from host setup, H2D, D2H, and total wall time.
- Report unsupported GPU layouts honestly rather than silently using a dense
  fallback.

Important differences from AdaSDF-CL v1.0.1:

- The original CUDA path queries dense or block-expanded grids from adaptive
  SDF packages.
- AdaSDF-CL v1.0.1-alpha can query global or block dense expansions through
  `QueryEngine`, but the current public expander samples analytic/demo adaptive
  box models rather than full compressed low-rank adaptive assets.
- Original UI GPU benchmark does not expose normal output support for these
  rows; AdaSDF-CL reports signed distance and finite-difference normal
  alignment for expanded data.
- Original all-points contact is a prototype, not a full compressed low-rank GPU
  contact solver.
- Both projects reject compressed-factor-direct GPU query instead of silently
  falling back to dense data. AdaSDF-CL v1.0.1 reports
  `CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.`
