# Tests

AdaSDF-CL uses CTest.

```bash
cmake -S . -B build -DADASDF_CL_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Test Categories

- Interface-only API shape tests.
- `.sdfbin` load/query tests when the existing core is available.
- Adaptive builder and writer round-trip tests when the existing builder bridge is available.
- CPU narrow-phase tests for candidate sampling, contact generation, contact reduction, and pair collision statistics.
- Placeholder tests for contact-only `.sdfbin` and surrogate recommendation behavior.

Tests that require unavailable optional backends skip gracefully. CUDA, FCL, Python, UI frameworks, raw datasets, and large model files are not required.
