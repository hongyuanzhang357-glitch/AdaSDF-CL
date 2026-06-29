# External Integration

AdaSDF-CL 0.7.0-alpha.2 installs a CMake package that can be consumed from an
external project without referencing the source tree.

## Build and Install

```bash
cmake -S . -B ../build/adasdf_cl-install-demo -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_BUILD_TESTS=ON
cmake --build ../build/adasdf_cl-install-demo --config Release
cmake --install ../build/adasdf_cl-install-demo --config Release --prefix ../build/adasdf_cl-install
```

The install tree contains:

- public headers under `include`;
- static library targets under the platform library directory;
- command-line tools under `bin`;
- CMake config and version files under `lib/cmake/AdaSDFCL`;
- `LICENSE`, `README.md`, and `CHANGELOG.md` under the documentation directory.

## Downstream CMake

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

`AdaSDFCL::adasdf_cl` is the installed runtime target. It carries the include
directories and links the implementation library. `AdaSDFCL::adasdf_cl_headers`
is also exported for header-only inspection and tooling.

## Validation

The package test and downstream example can be run together:

```bash
python scripts/run_install_validation.py --source . --build ../build/adasdf_cl_install_validation --install ../build/adasdf_cl_install --config Release
```

This script configures AdaSDF-CL without the existing core, installs it, builds
`tests/package`, then configures and runs `examples/downstream_cmake_project`.

## Optional Existing Core

`ADASDF_CL_USE_EXISTING_CORE=ON` is only needed when the build should sample or
write existing OctreeBlockLowRankSDF-backed `.sdfbin` assets. External package
consumers can compile, link, and use the public API without CUDA, FCL, Python, or
the existing core.

Core-free builds currently do not provide a standalone backend for generating,
reading, or querying real `.sdfbin` models. Real `.sdfbin` functionality
currently requires an existing-core enhanced build.

Downstream consumers of an existing-core enhanced install may need to add the
existing core's dependency prefix to `CMAKE_PREFIX_PATH`, for example a
vcpkg-installed dependency prefix that provides packages such as
`nlohmann_json`.

## Source Archives

CPack source archive generation is configured for ZIP and TGZ. Generated
archives should remain outside the committed source tree.
