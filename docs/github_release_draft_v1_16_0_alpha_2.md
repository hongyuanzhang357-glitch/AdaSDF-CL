# AdaSDF-CL v1.16.0-alpha.2

AdaSDF-CL v1.16.0-alpha.2 adds hierarchical sampling diagnostics and reduces
obvious overhead in the v1.16 hierarchical adaptive sampling path.

## Highlights

- Adds detailed hierarchical sampling diagnostics.
- Adds exact BVH query counters for diagnostics.
- Skips coarse probes for near-surface exact blocks.
- Reuses coarse samples for classification and prediction.
- Adds far-field quality-check modes and transition-specific quality-check
  samples.
- Adds `adasdf_sweep_hierarchical_sampling`.
- Adds a small project-generated `wavy_sphere_ascii.stl` benchmark fixture.

## Benchmark Boundary

Quality gates pass on the measured smoke fixtures, but v1.16.0-alpha.2 does
not yet demonstrate effective construction-speed acceleration.

Effective acceleration is only claimed when:

- `speedup > 1`;
- `quality_gate_passed=true`;
- `effective_speedup_claim_allowed=true`.

The measured alpha.2 fixed cases remain below speedup 1. The best wavy sphere
sweep case reached speedup 0.823 with quality passing.

## Compatibility

- Exact sampling remains the default.
- Hierarchical sampling remains opt-in.
- Near-surface blocks remain exact by default.
- Rejected predictions fall back to exact sampling.
- `.sdfbin` formats are unchanged.
- Older tags, including `v1.16.0-alpha` and `v1.16.0-alpha.1`, are not moved.

## Validation

- CPU-only build: PASS.
- Targeted hierarchical CTest subset: PASS, 9/9 tests.
- Python wrapper unittest: PASS, 54 tests with 1 environment-gated skip.
- Full CPU CTest: PASS, 189/189 tests.
- Install validation: PASS.
- Alpha validation with install validation: PASS.
- Clean check: PASS.
- `git diff --check`: PASS.
