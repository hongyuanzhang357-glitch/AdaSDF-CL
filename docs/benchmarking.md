# Batch Query Benchmarking

AdaSDF-CL v1.0.1-alpha includes a deterministic benchmark tool for batch
signed-distance, gradient, and normal queries across query backend and expansion
modes.

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

`--backend` is accepted as a legacy alias for `--query-backend`.

## Method

- Model: core-free demo adaptive analytic box SDF.
- Points: deterministic uniform samples inside the active domain.
- Seed: fixed by the point generator for reproducibility.
- CPU direct: `SDFModel::sampleDistance()` and `SDFModel::sampleGradient()` loop.
- CPU expanded: trilinear interpolation over `ExpandedSDF`.
- CUDA expanded: resident GPU global or block dense SDF query.
- Reference: CPU direct query at the same points.

The CUDA backend is optional. If CUDA is unavailable, CUDA rows are marked:

```text
status=skipped
error_message=CUDA backend unavailable
```

`CUDA + none` is invalid because compressed-direct CUDA query is not implemented.

## Output Fields

```text
query_backend
expansion_mode
selected_blocks
num_points
expanded_memory_mb
gpu_resident_memory_mb
setup_ms
query_kernel_ms
query_total_ms
ns_per_query
queries_per_second
fallback_count
max_abs_phi_error
max_normal_error
cuda_available
status
error_message
```

`query_kernel_ms` is `NA` for CPU rows. `query_total_ms` includes host-side work
needed by the query call. `setup_ms` includes expansion and resident upload when
those steps are required.

## Local v1.0.1 Snapshot

This snapshot was collected on the local CUDA validation machine with Release
builds, `ADASDF_CL_USE_EXISTING_CORE=OFF`, and CUDA enabled through an
ASCII-only source/build path. Values are representative, not portable
performance claims.

| Backend | Expansion | Blocks | Points | Setup ms | Kernel ms | Total ms | ns/query | Expanded MB | GPU MB | Max phi err | Max normal err |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CPU | None | all | 10,000 | 0.0024 | NA | 0.5152 | 51.52 | 0 | 0 | 0 | 0 |
| CPU | None | all | 100,000 | 0.0008 | NA | 5.0764 | 50.764 | 0 | 0 | 0 | 0 |
| CPU | None | all | 1,000,000 | 0.0009 | NA | 68.2956 | 68.2956 | 0 | 0 | 0 | 0 |
| CPU | Global | all | 10,000 | 4.5047 | NA | 1.7715 | 177.15 | 2.0001 | 0 | 0.004385 | 0.873865 |
| CPU | Global | all | 100,000 | 4.5588 | NA | 17.9953 | 179.953 | 2.0001 | 0 | 0.005406 | 0.873865 |
| CPU | Global | all | 1,000,000 | 4.6236 | NA | 187.9024 | 187.9024 | 2.0001 | 0 | 0.006410 | 0.899688 |
| CPU | Block | all | 10,000 | 3.4999 | NA | 2.0848 | 208.48 | 1.5006 | 0 | 0.010645 | 0.909407 |
| CPU | Block | all | 100,000 | 3.4537 | NA | 19.8335 | 198.335 | 1.5006 | 0 | 0.011475 | 2 |
| CPU | Block | all | 1,000,000 | 3.3005 | NA | 180.2792 | 180.2792 | 1.5006 | 0 | 0.014603 | 2 |
| CUDA | Global | all | 10,000 | 49.4015 | 1.1606 | 1.5861 | 158.61 | 2.0001 | 2.0001 | 0.004385 | 0.873865 |
| CUDA | Global | all | 100,000 | 5.8217 | 1.0820 | 5.2922 | 52.922 | 2.0001 | 2.0001 | 0.005406 | 0.873865 |
| CUDA | Global | all | 1,000,000 | 5.8224 | 10.4934 | 40.7194 | 40.7194 | 2.0001 | 2.0001 | 0.006410 | 0.899688 |
| CUDA | Block | all | 10,000 | 49.1817 | 1.3708 | 2.1142 | 211.42 | 1.5006 | 1.5005 | 0.010645 | 0.909407 |
| CUDA | Block | all | 100,000 | 5.43 | 7.7973 | 12.1534 | 121.534 | 1.5006 | 1.5005 | 0.011475 | 2 |
| CUDA | Block | all | 1,000,000 | 5.6849 | 79.9033 | 112.045 | 112.045 | 1.5006 | 1.5005 | 0.014603 | 2 |
| CUDA | Block | 0,1,2 | 10,000 | 51.0544 | 1.0605 | 1.3760 | 137.6 | 0.7503 | 0.7502 | 0.009609 | 0.889931 |
| CUDA | Block | 0,1,2 | 100,000 | 2.7632 | 3.5393 | 7.3181 | 73.181 | 0.7503 | 0.7502 | 0.011624 | 0.903103 |
| CUDA | Block | 0,1,2 | 1,000,000 | 2.6606 | 30.8537 | 59.8738 | 59.8738 | 0.7503 | 0.7502 | 0.011744 | 0.916844 |

The expanded SDF rows compare against CPU direct analytic values, so non-zero
phi and normal errors reflect finite-resolution dense-grid interpolation and
finite-difference normals. They should not be read as contact-quality claims.

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
