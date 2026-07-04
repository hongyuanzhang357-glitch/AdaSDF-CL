# Hierarchical Sampling Benchmarking

`adasdf_benchmark_hierarchical_sampling` compares exact adaptive SDF
construction against the v1.16.0-alpha hierarchical sampling path.

## Example

```bash
adasdf_benchmark_hierarchical_sampling model.stl \
  --max-level 4 \
  --block-resolution 8 \
  --coarse-resolution 3 \
  --quality-check-samples 3 \
  --transition-quality-check-samples 2 \
  --far-field-quality-check corners \
  --far-field-safety-factor 2.0 \
  --hierarchical-diagnostics \
  --target-error 1e-3 \
  --csv hierarchical_sampling.csv \
  --report hierarchical_sampling.md \
  --diagnostics-csv hierarchical_sampling_diagnostics.csv \
  --diagnostics-report hierarchical_sampling_diagnostics.md \
  --json hierarchical_sampling.json \
  --strict-json hierarchical_sampling_strict.json \
  --case-id hierarchical_sampling_v1_16
```

## Sweep Example

```bash
adasdf_sweep_hierarchical_sampling model.stl \
  --max-level 4 \
  --block-resolution 8 \
  --coarse-resolution 2,3,4 \
  --transition-quality-check-samples 1,2,3 \
  --far-field-quality-check corners,sparse \
  --far-field-safety-factor 2.0,3.0 \
  --target-error 1e-3 \
  --csv hierarchical_sampling_sweep.csv \
  --report hierarchical_sampling_sweep.md
```

## Metrics

The benchmark reports:

- exact build time;
- hierarchical build time;
- speedup;
- max, mean, RMS, and p95 error;
- sign mismatch counts;
- near-surface sign mismatch counts;
- exact sample counts;
- predicted sample counts;
- coarse sample reuse counts;
- quality-check sample counts;
- fallback block counts;
- diagnostics timing buckets;
- quality gate status;
- effective speedup claim status.

## Interpretation

Speedup is only meaningful when the quality gate passes, sign mismatch counts
remain acceptable, and `speedup > 1`. Some small fixtures may not show speedup
because the benchmark intentionally includes full construction and validation
overhead.
