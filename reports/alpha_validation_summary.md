# Alpha Validation Summary

- Source: `<source>`
- Build: `<build>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=ON -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
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
--   Version: 0.7.0-alpha.1
--   Build examples: ON
--   Build tests: ON
--   Existing core requested: ON
--   Existing core found: ON
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   CUDA: not required
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
  AABB.cpp
  AdaptiveBlockCompressor.cpp
  AdaptiveLowRankModel.cpp
  AllPointsSDFContact.cpp
  BlockPartition.cpp
  BVH.cpp
  BVHCandidateSampler.cpp
  BVHPairFilter.cpp
  ContactPointExporter.cpp
  ContactPipelineDecisionReport.cpp
  ContactSummaryReport.cpp
  DenseBlockGenerator.cpp
  DenseBlockCache.cpp
  FCLContactExporter.cpp
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/17 Test  #1: test_sdf_io ......................   Passed    0.05 sec
      Start  2: test_collision_query
 2/17 Test  #2: test_collision_query .............   Passed    0.03 sec
      Start  3: test_distance_query
 3/17 Test  #3: test_distance_query ..............   Passed    0.05 sec
      Start  4: test_collision_object
 4/17 Test  #4: test_collision_object ............   Passed    0.05 sec
      Start  5: test_pair_distance_query
 5/17 Test  #5: test_pair_distance_query .........   Passed    0.05 sec
      Start  6: test_pair_collision_query
 6/17 Test  #6: test_pair_collision_query ........   Passed    0.05 sec
      Start  7: test_candidate_point_sampler
 7/17 Test  #7: test_candidate_point_sampler .....   Passed    0.04 sec
      Start  8: test_contact_generator
 8/17 Test  #8: test_contact_generator ...........   Passed    0.04 sec
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

### Build Cube SDFBin: PASS

```bash
'<build>/Release/adasdf_build.exe' '<source>/tests/data/cube_closed_ascii.stl' '<workspace>/build/cube_v0_7_alpha.sdfbin' --near-surface-error 1e-4 --max-memory-mb 256 --compress
```

```text
AdaSDF-CL adaptive builder
Input mesh: <local-path>
Output sdfbin: <local-path>
Near-surface error: 0.0001
Max memory: 256 MB
Max octree level: 5
Compression: enabled
Surrogate recommendation: disabled
Build status: success
AABB min: -0.025 -0.025 -0.025
AABB max: 1.025 1.025 1.025
Memory footprint: 10432 bytes
Reload validation: success
```

### Load Query Example: PASS

```bash
'<build>/Release/adasdf_load_sdfbin_and_query.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
```

```text
AdaSDF-CL sdfbin query example
File: <local-path>
Valid: true
AABB min: -0.025 -0.025 -0.025
AABB max: 1.025 1.025 1.025
Memory footprint: 10432 bytes
Near-surface error: 0.00740882
Backend: existing sdf::AdaptiveLowRankModel CPU query

Query point: 0.5 0.5 0.5
Signed distance: -0.499912
Gradient: -3.38354e-15 0 0
Normal: 0 0 0
Gradient norm: 3.38354e-15

Query point: -0.025 -0.025 -0.025
Signed distance: 0.0442556
Gradient: -0.152103 -0.152103 -0.152103
...
```

### Pair Collision Example: PASS

```bash
'<build>/Release/adasdf_collision_between_two_objects.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
```

```text
AdaSDF-CL pair collision example
Model A: <local-path>
Model B: <local-path>
AABB A: min=(-0.025 -0.025 -0.025) max=(1.025 1.025 1.025)
AABB B: min=(-0.015 -0.025 -0.025) max=(1.035 1.025 1.025)
Query mode: Balanced
Backend: CPU narrow-phase: symmetric SDF-sampling research preview
Method: symmetric candidate-point SDF query + deterministic contact reduction
Colliding: true
Minimum distance: -0.489924
Candidate points: 606
Raw contacts: 2
Reduced contacts: 2
Contact count: 2
Contact[0].point: 0.5 0.5 0.5
Contact[0].normal: -1 2.77893e-15 -2.77893e-15
Contact[0].penetration_depth: 0.489924
Number of SDF queries: 4242
```

### Contact Reduction Demo: PASS

```bash
'<build>/Release/adasdf_contact_reduction_demo.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
```

```text
AdaSDF-CL contact reduction demo
Model: <local-path>

Scenario: separated
Offset: 1.25 0 0
Query mode: Balanced
Backend: CPU narrow-phase: symmetric SDF-sampling research preview
Method: AABB broadphase lower-bound distance
Distance: 0.2
Colliding: false
Candidate points: 0
Raw contacts: 0
Reduced contacts: 0
SDF queries: 0
Distance method: AABB broadphase lower-bound distance

Scenario: near_contact
Offset: 1 0 0
...
```

### Clean Check: PASS

```bash
'<local-path>' '<source>/scripts/check_repo_clean.py' '<source>'
```

```text
Repo clean check: PASS
```
