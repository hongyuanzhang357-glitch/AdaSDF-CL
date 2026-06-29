# AdaSDF-CL v1.0.0-alpha

## Highlights

- Added CPU batch query API for signed distance, gradient, and normal data.
- Added optional CUDA batch query backend for core-free analytic/demo adaptive boxes.
- Added CUDA box signed-distance and normal kernel, compiled only when CUDA is explicitly enabled and available.
- Added deterministic benchmark point generator.
- Added `adasdf_benchmark_batch_query` for 10k/100k/1M CPU and optional CUDA benchmarks.
- Added benchmark and GPU backend documentation.

## Validation

- CPU-only configure/build/test/install remains supported.
- CUDA unavailable behavior is a skip, not a failure.
- CPU tests run unconditionally.
- CUDA alignment tests run only when the CUDA backend is available.

## Important Boundaries

- CUDA is optional and disabled by default.
- v1.0.0-alpha CUDA support is limited to batch query for analytic/demo adaptive box SDFs.
- Full low-rank compressed SDF GPU expansion is planned but not yet complete.
- This release does not claim an industrial GPU collision pipeline.

## Suggested Benchmark Command

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --backend cpu,cuda --out benchmark.csv
```

If CUDA is unavailable:

```text
CUDA backend unavailable; skipping GPU benchmark.
```
