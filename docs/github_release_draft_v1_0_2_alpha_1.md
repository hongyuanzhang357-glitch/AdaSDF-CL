# AdaSDF-CL v1.0.2-alpha.1

CI portability hotfix for v1.0.2-alpha.

This pre-release supersedes the earlier `v1.0.2-alpha` tag by including the
Linux GitHub Actions benchmark CLI portability fix.

## Highlights from v1.0.2-alpha

- CUDA benchmark timing semantics
- Warmup / repeat benchmark controls
- Kernel-only timing
- Phi-only CUDA kernel
- Reuse-resident query workspace
- CPU/GPU benchmark fields for setup, kernel, total, copy, and postprocess
  timing

## Fixes in alpha.1

- Fixed Linux benchmark CLI test execution in GitHub Actions.
- Preserved Windows behavior.
- No core algorithm or public API changes.

## Benchmark interpretation

The reported `1.1594 ms / 1M points` result is a CUDA global phi-only kernel
timing. It is comparable to the original UI kernel-average timing. It is not
the full collision pipeline time.

Full query total time includes normal computation, host/device copies,
synchronization, CPU-side postprocessing, and output handling.

## Known limitations

- Full low-rank compressed SDF GPU-native query remains planned work.
- Full GPU contact generation and reduction remain planned work.
- Current CUDA acceleration targets expanded/demo SDF query primitives.
- This is an alpha / research-preview release.
