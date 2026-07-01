# Compression Quality Metrics

AdaSDF-CL v1.7.0-alpha records per-block and sampled model-level compression
quality metrics.

## Per-Block Metrics

Each compressed block stores:

- max absolute error;
- mean absolute error;
- RMS error;
- p95 absolute error;
- sign mismatch count;
- near-surface sign mismatch count;
- original and compressed byte counts;
- compression method and rank.

Dense fallback blocks report zero reconstruction loss because they keep the
original dense phi grid.

## Model-Level Audit

`CompressionQuality::compare()` samples deterministic points and compares a
reference dense adaptive model against a compressed model. It reports:

- sample count;
- max, mean, RMS, and p95 absolute error;
- sign mismatch count;
- near-surface sign mismatch count;
- compression ratio;
- warnings.

The CLI reports can be generated with:

```bash
adasdf_compress_adaptive_sdf adaptive.sdfbin compressed.sdfbin --quality-report quality.md
adasdf_build_compressed_sdf model.stl compressed.sdfbin --quality-report quality.md
```

## Interpretation

Compression error is separate from expanded-grid query error. A compressed SDF
can have low reconstruction error and still show expansion error if it is later
sampled into a coarse global or block `ExpandedSDF`.

v1.8.0-alpha build recommendation estimates compression memory and
near-surface error before a build, but those estimates do not replace this
quality audit. Always inspect the actual compression report and sampled quality
report after building a compressed asset.
