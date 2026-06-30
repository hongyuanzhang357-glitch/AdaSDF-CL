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
--   Version: 1.1.1-alpha
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
-- Generating done (0.4s)
...
```

### Build: PASS

```bash
cmake --build '<build>' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  test_docs_capability_links.vcxproj -> <build>\Release\test_docs_capability_links.exe
  test_query_engine_cuda.vcxproj -> <build>\Release\test_query_engine_cuda.exe
  test_surrogate_recommender.vcxproj -> <build>\Release\test_surrogate_recommender.exe
  test_demo_adaptive_builder.vcxproj -> <build>\Release\test_demo_adaptive_builder.exe
  test_cuda_query_workspace.vcxproj -> <build>\Release\test_cuda_query_workspace.exe
  adasdf_capabilities.vcxproj -> <build>\tools\Release\adasdf_capabilities.exe
  test_demo_surrogate_recommender.vcxproj -> <build>\Release\test_demo_surrogate_recommender.exe
  test_distance_query.vcxproj -> <build>\Release\test_distance_query.exe
  test_demo_sdfbin.vcxproj -> <build>\Release\test_demo_sdfbin.exe
  test_sdfmodel_query.vcxproj -> <build>\Release\test_sdfmodel_query.exe
  test_demo_adaptive_sdf_model.vcxproj -> <build>\Release\test_demo_adaptive_sdf_model.exe
  test_contact_reducer.vcxproj -> <build>\Release\test_contact_reducer.exe
  test_existing_core_expansion_bridge.vcxproj -> <build>\Release\test_existing_core_expansion_bridge.exe
  test_collision_svg_writer.vcxproj -> <build>\Release\test_collision_svg_writer.exe
  adasdf_recommend_demo.vcxproj -> <build>\Release\adasdf_recommend_demo.exe
  test_cuda_output_modes.vcxproj -> <build>\Release\test_cuda_output_modes.exe
  test_analytic_sdf_model.vcxproj -> <build>\Release\test_analytic_sdf_model.exe
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
-- Up-to-date: <install>/bin/adasdf_benchmark_batch_query.exe
-- Up-to-date: <install>/include
-- Up-to-date: <install>/include/adasdf
-- Up-to-date: <install>/include/adasdf/adapters
-- Up-to-date: <install>/include/adasdf/adapters/EigenAdapter.h
-- Up-to-date: <install>/include/adasdf/adapters/FCLAdapter.h
-- Up-to-date: <install>/include/adasdf/adapters/PythonBindingPlan.h
-- Up-to-date: <install>/include/adasdf/adasdf.h
...
```

### Package Configure: PASS

```bash
cmake -S '<source>/tests/package' -B '<local-path>' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_pkg
```

### Package Build: PASS

```bash
cmake --build '<local-path>' --config Release --parallel
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
AdaSDF-CL version: 1.1.1-alpha
Position: FCL-style SDF collision backend under development.
Boundary: complementary SDF backend, not a drop-in FCL replacement.

Implemented:
- core-free demo adaptive SDF
- FCL-style collision API
- point signed-distance, gradient, and normal query
- CPU direct/global/block query
- CUDA global/block expanded query when CUDA is enabled
- expansion quality audit
- sign mismatch and near-surface mismatch metrics
- SVG collision view
- CMake find_package integration

Experimental / partial:
- demo surrogate recommender
- contact reduction and research-preview contact output
- existing-core sampled expansion bridge
- CUDA expanded query backend
...
```

### Package Run: PASS

```bash
'<local-path>'
```

```text
AdaSDF-CL package consumer
Version: 1.1.1-alpha
Point: 1 2 3
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Batch query points: 2
CUDA batch backend available: false
CPU backend available: true
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<local-path>' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<local-path>' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_downstream.vcxproj -> <build>_ds\Release\adasdf_downstream.exe
```

### Downstream Run: PASS

```bash
'<local-path>'
```

```text
AdaSDF-CL downstream example
Version: 1.1.1-alpha
CPU backend: available
No .sdfbin supplied; running core-free demo adaptive path.
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Demo batch query points: 3
CUDA batch backend: unavailable
Demo colliding: true
Demo contacts: 4
```
