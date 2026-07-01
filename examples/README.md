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
- `11_capability_walkthrough.cpp`: v1.1.1 CPU-only walkthrough that prints version, query modes, a point query, a collision contact, and expansion quality metrics.
- `12_mesh_diagnostics_demo.cpp`: v1.2 CPU-only STL mesh diagnostics demo that reads an STL, prints a summary, and can write a Markdown report.
- `13_mesh_cleanup_demo.cpp`: v1.4 CPU-only safe cleanup walkthrough.
- `14_stl_to_dense_sdf_demo.cpp`: v1.5 CPU-only STL-to-uniform-DenseSDF workflow with reload, query, and collision.
- `15_adaptive_builder_preview_demo.cpp`: v1.5 adaptive builder interface preview that prints a plan and does not generate an adaptive `.sdfbin`.
- `downstream_cmake_project`: package-only external CMake smoke example with a no-input demo adaptive path.

## Existing-Core Examples

- `02_load_sdfbin_and_query.cpp`
- `03_collision_between_two_objects.cpp`
- `05_fcl_style_api.cpp`
- `06_build_then_query.cpp`
- `07_contact_reduction_demo.cpp`

These work when a compatible `.sdfbin` and query backend are available. Full
adaptive compressed STL-to-sdfbin construction still requires the existing core
or future standalone adaptive builder work.

## Preview-Only Examples

- `01_build_adaptive_sdf.cpp`
- `04_batched_gpu_query.cpp`
