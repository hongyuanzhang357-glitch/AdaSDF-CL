# Downstream CMake Project

This example is intentionally outside the main AdaSDF-CL build. It consumes an installed package only:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/adasdf-cl/install
cmake --build build --config Release
./build/adasdf_downstream
```

Windows multi-config generators place the executable under `build/Release/`.

The example links `AdaSDFCL::adasdf_cl`, prints the public version string, and touches the runtime CPU backend. With no arguments it creates an analytic demo box, runs a point query, and performs a small collision query. Passing a `.sdfbin` path exercises `SDFBinReader::read()` and model metadata access.

CUDA, FCL, Python, the existing research core, and the AdaSDF-CL source tree are not required for the package-only demo path.
