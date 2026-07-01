# Install Validation Summary

- Source: `<source>`
- Build: `<build>`
- Install: `<install>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_BENCHMARKS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON -DADASDF_CL_ENABLE_DEMO_BACKEND=ON -DADASDF_CL_ENABLE_DEMO_SURROGATE=ON -DADASDF_CL_ENABLE_COLLISION_VIEWER=ON -DADASDF_CL_ENABLE_CUDA=OFF '-DCMAKE_INSTALL_PREFIX=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
--
-- AdaSDF-CL configuration:
--   Version: 1.5.0-alpha
--   Build examples: ON
--   Build tests: ON
--   Benchmarks: ON
--   Existing core requested: OFF
--   Existing core found: OFF
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   Demo backend: ON
--   Demo surrogate: ON
--   Collision SVG viewer: ON
--   CUDA backend: OFF
--   CUDA toolkit: not found
--   FCL: not required
--
-- Configuring done (0.0s)
-- Generating done (0.6s)
...
```

### Build: PASS

```bash
cmake --build '<build>' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  test_stl_reader.vcxproj -> <build>\Release\test_stl_reader.exe
  test_query_mode_config.vcxproj -> <build>\Release\test_query_mode_config.exe
  adasdf_recommend_demo.vcxproj -> <build>\Release\adasdf_recommend_demo.exe
  test_surrogate_recommender.vcxproj -> <build>\Release\test_surrogate_recommender.exe
  test_sign_mismatch_metrics.vcxproj -> <build>\Release\test_sign_mismatch_metrics.exe
  test_sdfbin_writer_roundtrip.vcxproj -> <build>\Release\test_sdfbin_writer_roundtrip.exe
  test_distance_query.vcxproj -> <build>\Release\test_distance_query.exe
  test_sdfbin_real_load.vcxproj -> <build>\Release\test_sdfbin_real_load.exe
  adasdf_stl_to_dense_sdf_demo.vcxproj -> <build>\Release\adasdf_stl_to_dense_sdf_demo.exe
  test_sdf_io.vcxproj -> <build>\Release\test_sdf_io.exe
  adasdf_contact_reduction_demo.vcxproj -> <build>\Release\adasdf_contact_reduction_demo.exe
  test_query_engine_cuda.vcxproj -> <build>\Release\test_query_engine_cuda.exe
  adasdf_query_mode_demo.vcxproj -> <build>\tools\Release\adasdf_query_mode_demo.exe
  adasdf_build.vcxproj -> <build>\Release\adasdf_build.exe
  test_pair_distance_query.vcxproj -> <build>\Release\test_pair_distance_query.exe
  adasdf_core_free_demo_collision.vcxproj -> <build>\Release\adasdf_core_free_demo_collision.exe
  adasdf_mesh_cleanup_demo.vcxproj -> <build>\Release\adasdf_mesh_cleanup_demo.exe
...
```

### Install: PASS

```bash
cmake --install '<build>' --config Release --prefix '<install>'
```

```text
-- Up-to-date: <install>/lib/adasdf_cl_runtime.lib
-- Up-to-date: <install>/bin/adasdf_build.exe
-- Up-to-date: <install>/bin/adasdf_info.exe
-- Up-to-date: <install>/bin/adasdf_query.exe
-- Up-to-date: <install>/bin/adasdf_collide.exe
-- Up-to-date: <install>/bin/adasdf_make_demo_box.exe
-- Up-to-date: <install>/bin/adasdf_recommend_demo.exe
-- Up-to-date: <install>/bin/adasdf_build_demo_adaptive.exe
-- Up-to-date: <install>/bin/adasdf_collide_boxes_demo.exe
-- Up-to-date: <install>/bin/adasdf_query_mode_demo.exe
-- Up-to-date: <install>/bin/adasdf_expansion_quality.exe
-- Up-to-date: <install>/bin/adasdf_capabilities.exe
-- Up-to-date: <install>/bin/adasdf_mesh_check.exe
-- Up-to-date: <install>/bin/adasdf_mesh_clean.exe
-- Up-to-date: <install>/bin/adasdf_build_dense_sdf.exe
-- Up-to-date: <install>/bin/adasdf_build_adaptive_sdf_preview.exe
-- Up-to-date: <install>/bin/adasdf_benchmark_batch_query.exe
-- Up-to-date: <install>/include
-- Up-to-date: <install>/include/adasdf
-- Up-to-date: <install>/include/adasdf/adapters
...
```

### Package Configure: PASS

```bash
cmake -S '<source>/tests/package' -B '<workspace>/build/adasdf_cl_iv_pkg' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_pkg
```

### Package Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl_iv_pkg' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  test_find_package.vcxproj -> <build>_pkg\Release\test_find_package.exe
```

### Installed Capabilities CLI: PASS

```bash
'<install>/bin/adasdf_capabilities.exe' --verbose
```

```text
AdaSDF-CL version: 1.5.0-alpha
Position: FCL-style SDF collision backend under development.
Boundary: complementary SDF backend, not a drop-in FCL replacement.

Implemented:
- core-free demo adaptive SDF
- FCL-style collision API
- point signed-distance, gradient, and normal query
- CPU direct/global/block query
- CUDA global/block expanded query when CUDA is enabled
- ASCII and binary STL reader for mesh diagnostics
- ASCII STL writer for cleaned mesh export
- STL mesh diagnostics preflight report
- SDF build readiness scoring and repair suggestions
- safe mesh cleanup and before/after cleanup reports
- standalone uniform DenseSDF builder
- ADASDF_DENSE_SDFBIN_V1 read/write
- STL-to-DenseSDF public workflow
- expansion quality audit
- sign mismatch and near-surface mismatch metrics
...
```

### Installed Mesh Check CLI: PASS

```bash
'<install>/bin/adasdf_mesh_check.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --readiness --out '<workspace>/build/install_validation_mesh_report.md'
```

```text
AdaSDF-CL mesh diagnostics
Input: <local-path>
Format: ascii
Vertices: 8
Triangles: 12
Raw triangles: 12
AABB min: 0 0 0
AABB max: 1 1 1
Diagonal: 1.73205
Watertight: yes
Boundary edges: 0
Non-manifold edges: 0
Degenerate triangles: 0
Duplicate triangles: 0
Connected components: 1
Isolated vertices: 0
Recommendation: Mesh diagnostics passed. This is a preflight result, not a full self-intersection or SDF build guarantee.
SDF build readiness: Ready
Score: 100 / 100
Recommended for SDF build: yes
...
```

### Installed Mesh Clean CLI: PASS

```bash
'<install>/bin/adasdf_mesh_clean.exe' '<source>/tests/data/mesh_diagnostics/duplicate_and_degenerate_ascii.stl' '<workspace>/build/install_validation_cleaned.stl' --report '<workspace>/build/install_validation_cleanup_report.md'
```

```text
AdaSDF-CL safe mesh cleanup
Input vertices: 3
Input triangles: 3
Output vertices: 3
Output triangles: 1
Merged vertices: 0
Removed degenerate triangles: 1
Removed duplicate triangles: 1
Removed unused vertices: 0
Topology may have changed: yes
Warning: cleanup removed or merged mesh elements; rerun diagnostics and readiness before SDF construction
Output: <local-path>
Before readiness: NotRecommended, score 0
After readiness: Poor, score 65
Report: <local-path>

Validation note: mesh remained below Ready/Usable readiness after safe cleanup.
```

### Installed DenseSDF Build CLI: PASS

```bash
'<install>/bin/adasdf_build_dense_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_dense.sdfbin' --resolution 24 --padding 0.05 --report '<workspace>/build/install_validation_dense_report.md' --json '<workspace>/build/install_validation_dense_report.json'
```

```text
AdaSDF-CL dense SDF builder
Input: <local-path>
Output: <local-path>
Resolution: 24 x 24 x 24
Signed: yes
Watertight: yes
Triangles: 12
Build time ms: 21.2539
Memory bytes: 111128
Reload validation: success
Report: <local-path>
JSON report: <local-path>
```

### Installed DenseSDF Info CLI: PASS

```bash
'<install>/bin/adasdf_info.exe' '<workspace>/build/install_validation_dense.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.5.0-alpha
Path: <local-path>
Model name: uniform dense SDF
Valid: yes
Format: ADASDF_DENSE_SDFBIN_V1
DenseSDF resolution: 24 x 24 x 24
DenseSDF origin: -0.05 -0.05 -0.05
DenseSDF spacing: 0.0478261 0.0478261 0.0478261
DenseSDF signed: yes
Unit: m
Format version: 1
Query backend: core-free uniform DenseSDF backend
Query backend available: yes
AABB min: -0.05 -0.05 -0.05
AABB max: 1.05 1.05 1.05
Fine cell count: 23
Fine node count: 24
Fine spacing: 0.0478261
Max level: 0
...
```

### Installed DenseSDF Query CLI: PASS

```bash
'<install>/bin/adasdf_query.exe' '<workspace>/build/install_validation_dense.sdfbin' --point 0.5 0.5 0.5
```

```text
AdaSDF-CL point query
Path: <local-path>
Point: 0.5 0.5 0.5
Signed distance: -0.476087
Gradient: 0 0 0
Normal: 0 0 0
Query backend: core-free uniform DenseSDF backend
```

### Installed DenseSDF Collide CLI: PASS

```bash
'<install>/bin/adasdf_collide.exe' '<workspace>/build/install_validation_dense.sdfbin' '<workspace>/build/install_validation_dense.sdfbin' --max-contacts 4
```

```text
AdaSDF-CL collision query
Model A: <local-path>
Model B: <local-path>
Offset B: 0 0 0
AABB A: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
AABB B: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
Colliding: true
Minimum distance: -0.476087
Backend: CPU narrow-phase: symmetric SDF-sampling research preview
Requested query backend: cpu
Requested expansion: none
Requested blocks: all
Method: symmetric candidate-point SDF query + deterministic contact reduction
Candidate points: 462
Raw contacts: 2
Reduced contacts: 1
Requested max contacts: 4
Returned contacts: 1
Contact count: 1
Contact[0].point: 0.5 0.5 0.5
...
```

### Installed Adaptive Builder Preview CLI: PASS

```bash
'<install>/bin/adasdf_build_adaptive_sdf_preview.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_adaptive_preview.sdfbin' --target-error 1e-3 --memory-mb 512 --dry-run --plan '<workspace>/build/install_validation_adaptive_preview_plan.md'
```

```text
AdaSDF-CL adaptive builder interface preview
Input: <local-path>
Requested output: <local-path>
Dry run: yes
Valid plan: yes
Implemented in this version: no
Full adaptive compressed SDF build is not implemented in v1.5.0-alpha.
Plan: <local-path>
```

### Package Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_pkg/Release/test_find_package.exe'
```

```text
AdaSDF-CL package consumer
Version: 1.5.0-alpha
Point: 1 2 3
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Batch query points: 2
CUDA batch backend available: false
CPU backend available: true
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<workspace>/build/adasdf_cl_iv_ds' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl_iv_ds' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_downstream.vcxproj -> <build>_ds\Release\adasdf_downstream.exe
```

### Downstream Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_ds/Release/adasdf_downstream.exe'
```

```text
AdaSDF-CL downstream example
Version: 1.5.0-alpha
CPU backend: available
No .sdfbin supplied; running core-free demo adaptive path.
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Demo batch query points: 3
CUDA batch backend: unavailable
Demo colliding: true
Demo contacts: 4
```
