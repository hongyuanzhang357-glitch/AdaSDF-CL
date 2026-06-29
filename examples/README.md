# Examples

Build examples with:

```bash
cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

## Working Core-Free Examples

- `08_core_free_demo_collision.cpp`: v0.8 analytic box collision demo.
- `09_surrogate_adaptive_demo.cpp`: user target error and memory constraints -> demo surrogate -> demo adaptive SDF -> query.
- `10_two_box_collision_with_view.cpp`: build two demo adaptive boxes, collide, enforce `max_contacts`, and write an SVG collision view.
- `downstream_cmake_project`: package-only external CMake smoke example with a no-input demo adaptive path.

## Existing-Core Examples

- `02_load_sdfbin_and_query.cpp`
- `03_collision_between_two_objects.cpp`
- `05_fcl_style_api.cpp`
- `06_build_then_query.cpp`
- `07_contact_reduction_demo.cpp`

These work when a compatible `.sdfbin` and query backend are available. Full adaptive STL-to-sdfbin construction still requires the existing core or future standalone builder work.

## Preview-Only Examples

- `01_build_adaptive_sdf.cpp`
- `04_batched_gpu_query.cpp`
