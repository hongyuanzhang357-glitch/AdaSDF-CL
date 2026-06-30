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
--   Version: 1.4.0-alpha
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
-- Generating done (0.5s)
...
```

### Build: PASS

```bash
cmake --build '<build>' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  test_demo_adaptive_sdf_model.vcxproj -> <build>\Release\test_demo_adaptive_sdf_model.exe
  test_mesh_diagnostics_writer.vcxproj -> <build>\Release\test_mesh_diagnostics_writer.exe
  test_pair_distance_query.vcxproj -> <build>\Release\test_pair_distance_query.exe
  test_mesh_readiness.vcxproj -> <build>\Release\test_mesh_readiness.exe
  test_sdf_io.vcxproj -> <build>\Release\test_sdf_io.exe
  adasdf_core_free_demo_collision.vcxproj -> <build>\Release\adasdf_core_free_demo_collision.exe
  test_pair_collision_query.vcxproj -> <build>\Release\test_pair_collision_query.exe
  test_contact_only_sdfbin.vcxproj -> <build>\Release\test_contact_only_sdfbin.exe
  adasdf_expansion_quality.vcxproj -> <build>\tools\Release\adasdf_expansion_quality.exe
  adasdf_load_sdfbin_and_query.vcxproj -> <build>\Release\adasdf_load_sdfbin_and_query.exe
  test_stl_reader.vcxproj -> <build>\Release\test_stl_reader.exe
  adasdf_mesh_clean.vcxproj -> <build>\tools\Release\adasdf_mesh_clean.exe
  test_demo_surrogate_recommender.vcxproj -> <build>\Release\test_demo_surrogate_recommender.exe
  test_sdfbin_writer_roundtrip.vcxproj -> <build>\Release\test_sdfbin_writer_roundtrip.exe
  test_sdfmodel_query.vcxproj -> <build>\Release\test_sdfmodel_query.exe
  test_sdfbin_real_load.vcxproj -> <build>\Release\test_sdfbin_real_load.exe
  adasdf_build.vcxproj -> <build>\Release\adasdf_build.exe
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
-- Up-to-date: <install>/bin/adasdf_benchmark_batch_query.exe
-- Up-to-date: <install>/include
-- Up-to-date: <install>/include/adasdf
-- Up-to-date: <install>/include/adasdf/adapters
-- Up-to-date: <install>/include/adasdf/adapters/EigenAdapter.h
-- Up-to-date: <install>/include/adasdf/adapters/FCLAdapter.h
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
AdaSDF-CL version: 1.4.0-alpha
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
- expansion quality audit
- sign mismatch and near-surface mismatch metrics
- SVG collision view
- CMake find_package integration

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

### Package Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_pkg/Release/test_find_package.exe'
```

```text
AdaSDF-CL package consumer
Version: 1.4.0-alpha
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
Version: 1.4.0-alpha
CPU backend: available
No .sdfbin supplied; running core-free demo adaptive path.
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Demo batch query points: 3
CUDA batch backend: unavailable
Demo colliding: true
Demo contacts: 4
```
