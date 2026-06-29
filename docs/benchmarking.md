# Batch Query Benchmarking

AdaSDF-CL v1.0.0-alpha includes a deterministic benchmark tool for batch signed-distance, gradient, and normal queries.

## Command

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --backend cpu,cuda --out benchmark.csv
```

The CUDA backend is optional. If CUDA is unavailable, the tool keeps CPU rows and prints:

```text
CUDA backend unavailable; skipping GPU benchmark.
```

If `--backend cuda` is requested without `cpu` and CUDA is unavailable, the tool returns a non-zero skip code.

## Method

- Model: core-free analytic box SDF.
- Points: deterministic mixed distribution.
- Distribution: 50% near-surface shell and 50% uniform volume samples.
- Seed: fixed by the point generator for reproducibility.
- CPU baseline: `SDFModel::sampleDistance()` and `SDFModel::sampleGradient()` loop.
- CUDA backend: optional double-precision analytic box signed-distance and normal kernel.

## Output Fields

```text
backend
num_points
total_ms
ns_per_query
queries_per_second
max_abs_phi_error
max_normal_error
speedup_vs_cpu
cuda_available
```

## Standard Point Counts

```text
10,000
100,000
1,000,000
```

## Result Table

Use this table in benchmark reports:

```text
N points | CPU total ms | CPU ns/query | GPU total ms | GPU ns/query | Speedup | Max phi error | Max normal error
10,000
100,000
1,000,000
```

If CUDA is unavailable:

```text
GPU benchmark: SKIPPED because CUDA backend was unavailable on this machine.
```

## Hardware Notes

Benchmark results from different machines are not directly comparable. Report at least:

- CPU model;
- GPU model if CUDA is used;
- operating system;
- compiler;
- CMake configuration;
- AdaSDF-CL commit and tag.

Do not commit raw large benchmark logs. Small `.md` and `.csv` summaries are acceptable.
