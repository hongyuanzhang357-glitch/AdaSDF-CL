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
- `15_adaptive_builder_preview_demo.cpp`: adaptive builder planning preview; in v1.6 the real block-wise dense builder is `adasdf_build_adaptive_sdf`.
- `16_stl_to_adaptive_block_sdf_demo.cpp`: v1.6 CPU-only generated STL-to-adaptive-block-DenseSDF workflow with reload, query, and collision.
- `17_adaptive_vs_dense_sdf_demo.cpp`: v1.6 comparison between uniform DenseSDF and adaptive block-wise dense SDF on fixed sample points.
- `18_adaptive_block_compression_demo.cpp`: v1.7 adaptive block SDF compression demo with reload, query, collision, ratio, and error metrics.
- `19_dense_adaptive_compressed_comparison_demo.cpp`: v1.7 dense vs adaptive vs compressed comparison demo.
- `20_build_recommendation_demo.cpp`: v1.8 generated STL build recommendation demo; prints a recommended path, estimates, and CLI command without writing `.sdfbin`.
- `downstream_cmake_project`: package-only external CMake smoke example with a no-input demo adaptive path.

## Existing-Core Examples

- `02_load_sdfbin_and_query.cpp`
- `03_collision_between_two_objects.cpp`
- `05_fcl_style_api.cpp`
- `06_build_then_query.cpp`
- `07_contact_reduction_demo.cpp`

These work when a compatible `.sdfbin` and query backend are available. Full
Tucker/HOSVD compression, trained surrogate integration, and GPU-native
compressed query remain planned work. v1.8 deterministic build recommendation
is available through `adasdf_recommend_build`.

## Preview-Only Examples

- `01_build_adaptive_sdf.cpp`
- `04_batched_gpu_query.cpp`
