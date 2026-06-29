# External Integration

AdaSDF-CL 1.0.1-alpha installs a CMake package that can be consumed from an external project without referencing the source tree.

## Build and Install

```bash
cmake -S . -B build -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_BENCHMARKS=ON
cmake --build build --config Release
cmake --install build --config Release --prefix install
```

## Downstream CMake

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

The installed runtime exposes the demo surrogate, demo adaptive builder, demo adaptive reader/writer, collision API, CPU batch query API, query-mode API, optional CUDA resident expanded query API, and SVG writer.

## Package Validation

```bash
python scripts/run_install_validation.py --source . --build ../build/adasdf_cl_install_validation --install ../build/adasdf_cl_install --config Release
```

The downstream example builds a core-free demo adaptive box, runs point and CPU batch queries, and performs a collision query without CUDA, FCL, Python, or the existing core.

## Optional Existing Core

`ADASDF_CL_USE_EXISTING_CORE=ON` is only needed for existing-core adaptive `.sdfbin` assets. It is not required for the v1.0.1 demo adaptive, CPU query-mode, or CUDA expanded-query workflow.
