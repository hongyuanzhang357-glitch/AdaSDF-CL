# AdaSDF-CL v1.0.2-alpha CUDA Benchmark Semantics Report

Date: 2026-06-29

## v1.0.3-alpha Follow-Up

v1.0.3-alpha keeps the v1.0.2 timing boundary and adds explicit benchmark
output modes:

- `--output phi` for signed-distance-only timing;
- `--output phi,normal` for signed distance plus finite-difference normals;
- `--device-only` / `--no-download` for CUDA-side timing without result
  download or correctness checks.

The legacy `--phi-only` flag remains an alias for `--output phi`. Do not compare
phi-only kernel time with full query total time.

## 1. Goal

v1.0.2-alpha makes the v1.0.1 CUDA benchmark understandable, reproducible, and
comparable with original UI kernel-average reports. It does not implement full
low-rank compressed SDF native GPU query.

## 2. Added Files

- `tests/test_benchmark_timing_fields.cpp`
- `tests/test_benchmark_cli_modes.cpp`
- `tests/test_cuda_query_workspace.cpp`
- `tests/test_cuda_phi_only_kernel.cpp`
- `docs/cuda_benchmark_semantics_v1_0_2_report.md`
- `docs/github_release_draft_v1_0_2_alpha.md`

## 3. Modified Files

- `include/adasdf/version.h`
- `cmake/adasdf_cl_version.cmake`
- `include/adasdf/query/BatchQuery.h`
- `src/query/BatchQuery.cpp`
- `include/adasdf/query/QueryEngine.h`
- `src/query/QueryEngine.cpp`
- `include/adasdf/backend/CudaQueryBackend.h`
- `src/backend/CudaQueryBackend.cpp`
- `src/backend/CudaQueryKernels.cu`
- `benchmarks/adasdf_benchmark_batch_query.cpp`
- `CMakeLists.txt`
- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `docs/alpha_status.md`
- `docs/roadmap.md`
- `docs/benchmarking.md`
- `docs/gpu_backend.md`
- `docs/v1_0_1_cuda_performance_diagnosis.md`

## 4. Benchmark Timing Semantics

`BatchQueryTiming` now records:

```text
setup_ms, expand_ms, upload_sdf_ms, allocation_ms, h2d_points_ms, kernel_ms,
sync_ms, d2h_results_ms, postprocess_ms, free_ms, total_ms
```

`kernel_ms` is CUDA event elapsed time around the expanded SDF kernel.
`total_ms` is the full measured query call. For CUDA full query it includes
host output allocation, point packing and upload, kernel synchronization, result
download, CPU normal postprocess, and temporary allocation/free when the query
workspace is not reused.

The benchmark keeps legacy `query_kernel_ms` and `query_total_ms` aliases.

## 5. Resident Query Workspace Design

`CudaQueryWorkspace` owns reusable device buffers:

- query points;
- phi output;
- optional normal output.

`--reuse-resident` prepares this workspace before warmup/repeat loops and passes
it into `CudaResidentExpandedSDF::queryBatch()`. The SDF values and block
metadata were already resident in v1.0.1; v1.0.2 makes the query buffers
resident across repeats too.

## 6. Phi-Only Kernel Design

`expandedSdfPhiOnlyKernel` computes signed distance only. It does not compute
finite-difference normals and does not write normal buffers. Benchmark
`--phi-only` rows output `max_abs_phi_error` and mark `max_normal_error` as
`NA`.

## 7. Warmup And Repeat Design

- `--warmup N` runs uncounted queries before measurement.
- `--repeat N` runs counted queries and writes min/mean/max/std fields.
- `--kernel-only` derives `ns_per_query` from CUDA `kernel_mean_ms`.
- Without `--kernel-only`, `ns_per_query` is derived from `total_mean_ms`.

## 8. Kernel-Only Versus Total-Time

Original UI reports around 1.2 ms for 1M points are warmed kernel averages. The
closest AdaSDF-CL comparison is:

```bash
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --phi-only --kernel-only --reuse-resident --warmup 10 --repeat 50
```

Full AdaSDF-CL query rows include normals, copies, CPU postprocess, and output
allocation. They should not be compared directly with original UI kernel-only
numbers.

## 9. v1.0.1 Versus v1.0.2

| Area | v1.0.1-alpha | v1.0.2-alpha |
| --- | --- | --- |
| Timing | setup, query kernel, query total | formal timing breakdown plus min/mean/max/std |
| Warmup/repeat | not available | `--warmup`, `--repeat` |
| Kernel-only reporting | single CUDA event field | `--kernel-only` benchmark mode |
| Query buffers | per-query allocation/free | reusable `CudaQueryWorkspace` with `--reuse-resident` |
| Phi-only | not available | `--phi-only` CUDA kernel path |
| Block lookup | linear scan for all samples | single-block direct path, neighbor samples prefer center block |

## 10. Original UI Comparison

The original UI global dense 1M kernel average was about 1.24 ms. On the local
v1.0.2 CUDA build, AdaSDF-CL global phi-only kernel mean is 1.1594 ms, which
indicates the large v1.0.1 headline gap was mostly a timing-scope and normal
kernel mismatch for the global phi-query comparison.

Block mode still has a kernel gap because selected-block lookup remains more
expensive than global dense lookup.

## 11. Benchmark Table

Release CUDA build, ASCII-only source/build path, demo adaptive analytic box,
1M points unless noted.

| Scenario | Points | Mode | Kernel mean ms | Total mean ms | ns/query basis | Expanded MB | GPU resident MB | Max phi err | Max normal err |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| CUDA global phi-only kernel | 1,000,000 | `--phi-only --kernel-only --reuse-resident --warmup 10 --repeat 50` | 1.1594 | 13.7921 | 1.1594 | 2.0001 | 32.5177 | 0.006410 | NA |
| CUDA global full total | 1,000,000 | `--reuse-resident --warmup 10 --repeat 50` | 11.0974 | 42.5593 | 42.5593 | 2.0001 | 55.4058 | 0.006410 | 0.899688 |
| CUDA block 0,1,2 phi-only kernel | 1,000,000 | `--blocks 0,1,2 --phi-only --kernel-only --reuse-resident --warmup 10 --repeat 50` | 4.1679 | 15.3624 | 4.1679 | 0.7503 | 31.2678 | 0.011744 | NA |
| CPU none | 1,000,000 | `--repeat 10` | NA | 60.5139 | 60.5139 | 0 | 0 | 0 | 0 |

## 12. Current Bottlenecks

For global phi-only kernel comparison, the kernel is now close to the original
UI reference. For full global query, the main remaining costs are normal
finite differences, H2D/D2H transfers, CPU normal postprocess, and output
allocation. For block mode, the remaining kernel cost is block lookup and branch
divergence even after the minimal same-block preference.

## 13. Next Optimization Recommendations

- Add pinned host memory for point/result transfers.
- Add SoA point/normal layout experiments.
- Add optional float-mode benchmark rows while keeping double precision default.
- Add a denser block lookup structure instead of linear scan.
- Add a full dense/block SDF backend that mirrors original UI assets more
  closely.
- Add an existing-core expanded asset bridge for real `.sdfbin` benchmark
  parity.

## 14. Validation

CPU-only validation:

```text
cmake configure: PASS
cmake --build Release: PASS
ctest Release: 41/41 PASS
```

CUDA validation through ASCII-only source/build path:

```text
cmake configure: PASS, CUDA backend ON, CUDA toolkit found
cmake --build Release: PASS
ctest Release: 41/41 PASS
```

No old tags were moved. No force push was used.
