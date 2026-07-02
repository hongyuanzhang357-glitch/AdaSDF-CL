# AdaSDF-CL v1.8.1-alpha

AdaSDF-CL v1.8.1-alpha adds a lightweight pure-Python CLI wrapper.

## Added

- Pure-Python `adasdf_cli` package under `python/adasdf_cli`.
- Standard-library-only subprocess runner for installed AdaSDF-CL CLI tools.
- Python helpers for mesh check, mesh cleanup, build recommendation, DenseSDF
  build, AdaptiveBlockSDF build, compressed SDF build, info, query, collide,
  expansion quality, and benchmark.
- Dataclass result types preserving command, stdout, stderr, return code, and
  elapsed time.
- Best-effort parsers for common CLI output fields.
- Convenience workflows for recommend-then-build and preprocess-then-build.
- Unittest-based Python coverage and optional real CLI smoke tests.

## Notes

- This is not a native pybind11 binding.
- This is not a C++ extension module.
- The wrapper requires installed AdaSDF-CL CLI tools.
- Runtime dependencies are limited to the Python standard library.
- Native Python bindings remain planned work.

## Validation

- CPU CTest target: expected PASS.
- CUDA CTest target: expected PASS when CUDA is available.
- Install validation: expected PASS, including Python wrapper tests with
  installed tools.
- Alpha validation: expected PASS.
- Clean check: expected PASS.
