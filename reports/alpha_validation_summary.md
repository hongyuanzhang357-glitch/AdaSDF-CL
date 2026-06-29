# Alpha Validation Summary

- Source: `<source>`
- Build: `<build>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON -DADASDF_CL_ENABLE_DEMO_BACKEND=ON
```

```text
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- The CXX compiler identification is MSVC 19.42.34444.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
--
-- AdaSDF-CL configuration:
--   Version: 0.8.0-alpha
--   Build examples: ON
--   Build tests: ON
--   Existing core requested: OFF
--   Existing core found: OFF
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   Core-free demo backend: ON
...
```

### Build: PASS

```bash
cmake --build '<build>' --config Release
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/CMakeLists.txt
  FCLAdapter.cpp
  Backend.cpp
  GpuBackend.cpp
  CompressedSDF.cpp
  AdaptiveSDFBuilder.cpp
  ExistingBuilderBridge.cpp
  SDFBuilder.cpp
  SurrogateRecommender.cpp
  AnalyticSDFModel.cpp
  MeshModel.cpp
  SDFModel.cpp
  Transform.cpp
  ContactOnlySDFBin.cpp
  DemoSDFBin.cpp
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/21 Test  #1: test_sdf_io ......................   Passed    0.01 sec
      Start  2: test_collision_query
 2/21 Test  #2: test_collision_query .............   Passed    0.01 sec
      Start  3: test_distance_query
 3/21 Test  #3: test_distance_query ..............   Passed    0.01 sec
      Start  4: test_collision_object
 4/21 Test  #4: test_collision_object ............   Passed    0.02 sec
      Start  5: test_pair_distance_query
 5/21 Test  #5: test_pair_distance_query .........   Passed    0.01 sec
      Start  6: test_pair_collision_query
 6/21 Test  #6: test_pair_collision_query ........   Passed    0.01 sec
      Start  7: test_candidate_point_sampler
 7/21 Test  #7: test_candidate_point_sampler .....   Passed    0.01 sec
      Start  8: test_contact_generator
 8/21 Test  #8: test_contact_generator ...........   Passed    0.01 sec
      Start  9: test_contact_reducer
...
```

### Install Validation: PASS

```bash
'<local-path>' '<source>/scripts/run_install_validation.py' --source '<source>' --build '<workspace>/build/adasdf_cl_iv' --install '<workspace>/build/adasdf_cl_install' --config Release
```

```text
[install-validation] Configure
[install-validation] Build
[install-validation] Install
[install-validation] Package Configure
[install-validation] Package Build
[install-validation] Package Run
[install-validation] Downstream Configure
[install-validation] Downstream Build
[install-validation] Downstream Run
Install validation: PASS
Report: reports/install_validation_summary.md
```

### Make Demo Box SDFBin: PASS

```bash
'<build>/Release/adasdf_make_demo_box.exe' '<workspace>/build/cube_demo_v0_8.sdfbin'
```

```text
AdaSDF-CL demo box generator
Output: <local-path>
Shape: box
Center: 0 0 0
Half extent: 0.5 0.5 0.5
Backend: core-free analytic demo SDF backend
Write status: success
Reload validation: success
```

### Demo Info CLI: PASS

```bash
'<build>/Release/adasdf_info.exe' '<workspace>/build/cube_demo_v0_8.sdfbin'
```

```text
AdaSDF-CL info
Library version: 0.8.0-alpha
Path: <local-path>
Model name: analytic demo box
Valid: yes
Format: ADASDF_DEMO_SDFBIN_V1
Shape: box
Center: 0 0 0
Half extent: 0.5 0.5 0.5
Unit: m
Format version: 1
Query backend: core-free analytic demo SDF backend
Query backend available: yes
AABB min: -0.5 -0.5 -0.5
AABB max: 0.5 0.5 0.5
Fine cell count: 0
Fine node count: 0
Fine spacing: 0
...
```

### Demo Query CLI: PASS

```bash
'<build>/Release/adasdf_query.exe' '<workspace>/build/cube_demo_v0_8.sdfbin' --point 0 0 0
```

```text
AdaSDF-CL point query
Path: <local-path>
Point: 0 0 0
Signed distance: -0.5
Gradient: 1 0 0
Normal: 1 0 0
Query backend: core-free analytic demo SDF backend
```

### Demo Collide CLI: PASS

```bash
'<build>/Release/adasdf_collide.exe' '<workspace>/build/cube_demo_v0_8.sdfbin' '<workspace>/build/cube_demo_v0_8.sdfbin' --max-contacts 4
```

```text
AdaSDF-CL collision query
Model A: <local-path>
Model B: <local-path>
Offset B: 0 0 0
AABB A: min=(-0.5 -0.5 -0.5) max=(0.5 0.5 0.5)
AABB B: min=(-0.5 -0.5 -0.5) max=(0.5 0.5 0.5)
Colliding: true
Minimum distance: -0.5
Backend: CPU narrow-phase: symmetric SDF-sampling research preview
Method: symmetric candidate-point SDF query + deterministic contact reduction
Candidate points: 606
Raw contacts: 606
Reduced contacts: 4
Contact count: 4
Contact[0].point: 0 0 0
Contact[0].normal: 1 0 0
Contact[0].penetration_depth: 0.5
Contact[0].signed_distance: -0.5
...
```

### Core-Free Demo Collision Example: PASS

```bash
'<build>/Release/adasdf_core_free_demo_collision.exe'
```

```text
AdaSDF-CL core-free demo collision
Demo sdfbin: <local-path>
Backend: core-free analytic demo SDF backend
Colliding: true
Minimum distance: -0.25
Distance query: -0.25
Contact count: 4
Contact[0].point: 0 0 0
Contact[0].normal: -1 0 0
Contact[0].penetration_depth: 0.25
Method: symmetric candidate-point SDF query + deterministic contact reduction
```

### Clean Check: PASS

```bash
'<local-path>' '<source>/scripts/check_repo_clean.py' '<source>'
```

```text
Repo clean check: PASS
```
