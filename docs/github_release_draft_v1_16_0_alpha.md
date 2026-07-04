# AdaSDF-CL v1.16.0-alpha

AdaSDF-CL v1.16.0-alpha adds experimental hierarchical adaptive sampling with
a quality guard for adaptive and compressed SDF builders.

## Highlights

- New `include/adasdf/sampling` and `src/sampling` module.
- Opt-in `--sampling hierarchical` on adaptive and compressed builder CLIs.
- Near-surface blocks remain exact by default.
- Predicted blocks require quality guard checks and exact fallback.
- New `adasdf_benchmark_hierarchical_sampling` speed/quality benchmark.
- Python wrapper support for hierarchical sampling options and benchmark
  parsing.
- Install and alpha validation coverage for the new benchmark path.

## Boundaries

- Exact sampling remains the default.
- `.sdfbin` formats are unchanged.
- `--no-quality-guard` does not enable unsafe prediction; the policy falls back
  to exact sampling.
- This release does not implement Tucker/HOSVD, GPU-native compressed query,
  native Python bindings, ROS/MoveIt integration, FCL fallback, CCD, or a
  contact solver.

## Validation

Release validation should include CPU-only build, full CTest, install
validation, alpha validation, Python wrapper tests, repo clean check, and tag
CI on `v1.16.0-alpha`.
