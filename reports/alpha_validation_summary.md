# Alpha Validation Summary

- Source: `<source>`
- Build: `<build>`
- Config: `Debug`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=ON -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
--
-- AdaSDF-CL configuration:
--   Version: 0.7.0-alpha
--   Build examples: ON
--   Build tests: ON
--   Existing core requested: ON
--   Existing core found: ON
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   CUDA: not required
--   FCL: not required
--
-- Configuring done (0.0s)
-- Generating done (0.2s)
-- Build files have been written to: <build>
```

### Build: PASS

```bash
cmake --build '<build>' --config Debug
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_existing_core.vcxproj -> <build>\Debug\adasdf_existing_core.lib
  adasdf_cl_runtime.vcxproj -> <build>\Debug\adasdf_cl_runtime.lib
  adasdf_build.vcxproj -> <build>\Debug\adasdf_build.exe
  adasdf_build_then_query.vcxproj -> <build>\Debug\adasdf_build_then_query.exe
  adasdf_collide.vcxproj -> <build>\Debug\adasdf_collide.exe
  adasdf_collision_between_two_objects.vcxproj -> <build>\Debug\adasdf_collision_between_two_objects.exe
  adasdf_contact_reduction_demo.vcxproj -> <build>\Debug\adasdf_contact_reduction_demo.exe
  adasdf_fcl_style_api.vcxproj -> <build>\Debug\adasdf_fcl_style_api.exe
  adasdf_info.vcxproj -> <build>\Debug\adasdf_info.exe
  adasdf_load_sdfbin_and_query.vcxproj -> <build>\Debug\adasdf_load_sdfbin_and_query.exe
  adasdf_query.vcxproj -> <build>\Debug\adasdf_query.exe
  test_adaptive_builder.vcxproj -> <build>\Debug\test_adaptive_builder.exe
  test_build_options.vcxproj -> <build>\Debug\test_build_options.exe
  test_candidate_point_sampler.vcxproj -> <build>\Debug\test_candidate_point_sampler.exe
  test_collision_object.vcxproj -> <build>\Debug\test_collision_object.exe
  test_collision_query.vcxproj -> <build>\Debug\test_collision_query.exe
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Debug --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/17 Test  #1: test_sdf_io ......................   Passed    0.02 sec
      Start  2: test_collision_query
 2/17 Test  #2: test_collision_query .............   Passed    0.01 sec
      Start  3: test_distance_query
 3/17 Test  #3: test_distance_query ..............   Passed    0.01 sec
      Start  4: test_collision_object
 4/17 Test  #4: test_collision_object ............   Passed    0.03 sec
      Start  5: test_pair_distance_query
 5/17 Test  #5: test_pair_distance_query .........   Passed    1.38 sec
      Start  6: test_pair_collision_query
 6/17 Test  #6: test_pair_collision_query ........   Passed    1.40 sec
      Start  7: test_candidate_point_sampler
 7/17 Test  #7: test_candidate_point_sampler .....   Passed    0.66 sec
      Start  8: test_contact_generator
 8/17 Test  #8: test_contact_generator ...........   Passed    0.80 sec
      Start  9: test_contact_reducer
...
```

### Install Validation: PASS

```bash
'<local-path>' '<source>/scripts/run_install_validation.py' --source '<source>' --build '<workspace>/build/adasdf_cl_iv' --install '<workspace>/build/adasdf_cl_install' --config Debug
```

```text
Install validation: PASS
Report: reports/install_validation_summary.md
```

### Build Cube SDFBin: PASS

```bash
'<build>/Debug/adasdf_build.exe' '<source>/tests/data/cube_closed_ascii.stl' '<workspace>/build/cube_v0_7_alpha.sdfbin' --near-surface-error 1e-4 --max-memory-mb 256 --compress
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
'<build>/Debug/adasdf_load_sdfbin_and_query.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
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
'<build>/Debug/adasdf_collision_between_two_objects.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
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
'<build>/Debug/adasdf_contact_reduction_demo.exe' '<workspace>/build/cube_v0_7_alpha.sdfbin'
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
