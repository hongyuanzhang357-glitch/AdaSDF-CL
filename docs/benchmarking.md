# Batch Query Benchmarking

AdaSDF-CL v1.0.3-alpha includes a deterministic benchmark tool for batch
signed-distance, gradient, and normal queries across query backend and expansion
modes. It also separates full-query timing from CUDA kernel timing so results
can be compared honestly with original UI kernel-average reports.

## Commands

CPU direct baseline:

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion none --out cpu_direct.csv
```

CPU expanded paths:

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion global --out cpu_global.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion block --blocks all --out cpu_block_all.csv
```

CUDA resident expanded paths:

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion global --out cuda_global.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion block --blocks all --out cuda_block_all.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion block --blocks 0,1,2 --out cuda_blocks.csv
```

Original UI style kernel comparison:

```bash
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_kernel.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi,normal --reuse-resident --warmup 10 --repeat 50 --out cuda_global_full.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --device-only --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_device_only.csv
```

`--backend` is accepted as a legacy alias for `--query-backend`.

## CLI Modes

- `--warmup N`: run N uncounted query iterations before measurement.
- `--repeat N`: run N counted query iterations and report min/mean/max/std.
- `--kernel-only`: derive `ns_per_query` and throughput from CUDA `kernel_ms`.
- `--reuse-resident`: reuse CUDA query points, phi, and normal buffers across
  warmup/repeat iterations.
- `--output phi`: compute signed distance only and skip finite-difference normals.
- `--output phi,normal`: compute signed distance plus finite-difference normals.
- `--phi-only`: legacy alias for `--output phi`.
- `--device-only` / `--no-download`: keep CUDA results on device after kernel
  synchronization. This skips D2H download and correctness checks.

`--kernel-only` is meaningful only for CUDA rows. CPU rows mark CUDA kernel
fields as `NA`.

Phi-only kernel timing is closest to original UI kernel-average comparison.
Phi+normal is the full query primitive currently used by contact workflows.
Device-only mode is a performance-analysis mode, not a correctness mode.

## Benchmark Timing Semantics

`total_ms` is not the same measurement as `kernel_ms`.

| Field | Scope |
| --- | --- |
| `setup_ms` | One-time setup before query repeats. For expanded modes this includes CPU SDF expansion and, for CUDA, resident SDF upload. |
| `expand_ms` | CPU `SDFExpander` time. |
| `upload_sdf_ms` | CUDA upload time for expanded SDF blocks and values. |
| `allocation_ms` | Per-query output allocation and temporary CUDA query-buffer allocation. With `--reuse-resident`, query-buffer allocation should disappear after warmup. |
| `h2d_points_ms` | Host point packing plus H2D point copy. |
| `kernel_ms` | CUDA event elapsed time around the expanded SDF kernel only. |
| `sync_ms` | Host wait around CUDA event synchronization. |
| `d2h_results_ms` | Device-to-host copy for phi and, unless `--output phi`, normals. |
| `postprocess_ms` | CPU conversion of downloaded normal vectors into output gradients/normals. |
| `free_ms` | Per-query temporary CUDA query-buffer free time. |
| `total_ms` | Full measured query call time for one repeat. |

Original UI CUDA reports that state "kernel average" should be compared to
AdaSDF-CL `kernel_ms`, preferably from `--kernel-only --output phi
--reuse-resident --warmup N --repeat N`. They should not be compared directly
to `total_ms`.

## Output Fields

The CSV keeps v1.0.1 compatibility aliases and adds v1.0.2 timing fields:

```text
query_backend
expansion_mode
selected_blocks
num_points
expanded_memory_mb
gpu_resident_memory_mb
setup_ms
expand_ms
upload_sdf_ms
allocation_ms
h2d_points_ms
kernel_ms
sync_ms
d2h_results_ms
postprocess_ms
free_ms
total_ms
query_kernel_ms
query_total_ms
ns_per_query
queries_per_second
fallback_count
max_abs_phi_error
max_normal_error
cuda_available
warmup
repeat
kernel_min_ms
kernel_mean_ms
kernel_max_ms
kernel_std_ms
total_min_ms
total_mean_ms
total_max_ms
total_std_ms
output_mode
phi_only
reuse_resident
kernel_only
workspace_reused
allocation_count
workspace_capacity
workspace_device_memory_mb
block_lookup_count
block_scan_count
center_block_hit_rate
neighbor_same_block_rate
download_results
correctness_checked
host_memory
layout
status
error_message
```

`query_kernel_ms` and `query_total_ms` are retained as aliases for existing
validation scripts. `phi_only` is retained as a compatibility column.
`max_normal_error` is `NA` for `--output phi` rows. `correctness_checked=false`
means the row skipped result download, normally because `--device-only` was set.

## Method

- Model: core-free demo adaptive analytic box SDF.
- Points: deterministic uniform samples inside the active domain.
- Seed: fixed by the point generator for reproducibility.
- CPU direct: `SDFModel::sampleDistance()` and `SDFModel::sampleGradient()` loop.
- CPU expanded: trilinear interpolation over `ExpandedSDF`.
- CUDA expanded: resident GPU global or block dense SDF query.
- Reference: CPU direct query at the same points.

Selected block mode is intended for local contact-region point distributions.
For global uniform point clouds, selected blocks can be slower or less
representative because many points may require fallback handling or miss the
selected region.

The CUDA backend is optional. If CUDA is unavailable, CUDA rows are marked:

```text
status=skipped
error_message=CUDA backend unavailable
```

`CUDA + none` is invalid because compressed-direct CUDA query is not
implemented.

## Hardware Notes

Benchmark results from different machines are not directly comparable. Report
at least:

- CPU model;
- GPU model if CUDA is used;
- operating system;
- compiler;
- CMake configuration;
- AdaSDF-CL commit and tag.

Do not commit raw large benchmark logs. Small `.md` and `.csv` summaries are
acceptable when they support a release report.
