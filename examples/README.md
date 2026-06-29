# Examples

Build examples with:

```bash
cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

Executable paths depend on the CMake generator and configuration.

## 01_build_adaptive_sdf.cpp

Status: API preview.

Shows the intended adaptive builder API shape. Prefer `06_build_then_query.cpp` or `adasdf_build` for a working STL build path in the current alpha.

## 02_load_sdfbin_and_query.cpp

Status: working when a compatible `.sdfbin` and existing-core bridge are available.

Expected command:

```bash
adasdf_load_sdfbin_and_query model.sdfbin
```

Key output fields:

- validity;
- AABB;
- memory footprint;
- query backend;
- signed distance;
- finite-difference gradient;
- normal.

## 03_collision_between_two_objects.cpp

Status: working when a compatible `.sdfbin` is available.

Expected command:

```bash
adasdf_collision_between_two_objects model.sdfbin
```

Optional second model path and translation can be supplied. Key output fields:

- backend and method;
- candidate point count;
- raw and reduced contact counts;
- minimum distance;
- first contact normal and penetration depth.

## 04_batched_gpu_query.cpp

Status: API preview.

CUDA batched pair query is not implemented in 0.7.0-alpha.2.

## 05_fcl_style_api.cpp

Status: working FCL-style wrapper surface, not FCL ABI compatibility.

Expected command:

```bash
adasdf_fcl_style_api model.sdfbin
```

## 06_build_then_query.cpp

Status: working when the existing builder bridge is available.

Expected command:

```bash
adasdf_build_then_query input.stl output.sdfbin
```

Builds from STL, writes `.sdfbin`, reloads, and samples a few points.

## 07_contact_reduction_demo.cpp

Status: working when a compatible `.sdfbin` is available.

Expected command:

```bash
adasdf_contact_reduction_demo model.sdfbin
```

Runs separated, near-contact, and penetrating cube scenarios and prints candidate, raw contact, reduced contact, distance, normal, and penetration statistics.

## downstream_cmake_project

Status: working install-tree smoke example.

Build this directory outside the AdaSDF-CL source tree after installing the
package:

```bash
cmake -S examples/downstream_cmake_project -B downstream-build -DCMAKE_PREFIX_PATH=/path/to/adasdf-cl/install
cmake --build downstream-build --config Release
```

The example links `AdaSDFCL::adasdf_cl` and does not require CUDA, FCL, Python,
or the AdaSDF-CL source tree after installation.
