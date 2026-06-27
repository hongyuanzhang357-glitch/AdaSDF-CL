# AdaSDF-CL 0.7.0-alpha Release Notes

AdaSDF-CL 0.7.0-alpha focuses on external integration and packaging. The main
goal is that another CMake project can consume an installed AdaSDF-CL tree with:

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

## Highlights

- `AdaSDFCL::adasdf_cl` is the installed runtime target.
- `AdaSDFCL::adasdf_cl_headers` is exported for header-only inspection.
- `scripts/run_install_validation.py` configures, builds, installs, and runs
  package/downstream smoke checks.
- `tests/package` verifies package config and runtime linking.
- `examples/downstream_cmake_project` demonstrates source-tree-independent
  consumption.
- `adasdf_info`, `adasdf_query`, and `adasdf_collide` are installed alongside
  `adasdf_build`.
- CI runs install/downstream validation with the existing core disabled.
- CPack source archive metadata is configured.

## Compatibility

The package-only smoke path does not require CUDA, FCL, Python, or the existing
OctreeBlockLowRankSDF core. Loading and sampling real existing-core `.sdfbin`
files still requires `ADASDF_CL_USE_EXISTING_CORE=ON` at build time.

## Not Included

- CUDA batched query implementation.
- FCL ABI-compatible adapter.
- Python bindings.
- Industrial certified contact solver.
- Real DASH-SDF surrogate inference backend.
