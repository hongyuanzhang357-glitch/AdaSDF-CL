# Alpha Validation Summary

- Source: `<source>`
- Build: `<build>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_BENCHMARKS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON -DADASDF_CL_ENABLE_DEMO_BACKEND=ON -DADASDF_CL_ENABLE_DEMO_SURROGATE=ON -DADASDF_CL_ENABLE_COLLISION_VIEWER=ON -DADASDF_CL_ENABLE_CUDA=ON
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
-- Looking for a CUDA compiler
-- Looking for a CUDA compiler - NOTFOUND
CMake Warning at CMakeLists.txt:72 (message):
  ADASDF_CL_ENABLE_CUDA=ON but no CUDA compiler was found.  Building without
  CUDA batch query kernels.


--
-- AdaSDF-CL configuration:
--   Version: 1.0.2-alpha
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
  CudaQueryBackend.cpp
  GpuBackend.cpp
  PointCloudGenerator.cpp
  CompressedSDF.cpp
  AdaptiveSDFBuilder.cpp
  DemoAdaptiveSDFBuilder.cpp
  DemoSurrogateRecommender.cpp
  ExistingBuilderBridge.cpp
  SDFBuilder.cpp
  SurrogateRecommender.cpp
  AnalyticSDFModel.cpp
  DemoAdaptiveSDFModel.cpp
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/41 Test  #1: test_sdf_io .......................   Passed    0.01 sec
      Start  2: test_collision_query
 2/41 Test  #2: test_collision_query ..............   Passed    0.01 sec
      Start  3: test_distance_query
 3/41 Test  #3: test_distance_query ...............   Passed    0.02 sec
      Start  4: test_collision_object
 4/41 Test  #4: test_collision_object .............   Passed    0.01 sec
      Start  5: test_pair_distance_query
 5/41 Test  #5: test_pair_distance_query ..........   Passed    0.02 sec
      Start  6: test_pair_collision_query
 6/41 Test  #6: test_pair_collision_query .........   Passed    0.01 sec
      Start  7: test_candidate_point_sampler
 7/41 Test  #7: test_candidate_point_sampler ......   Passed    0.01 sec
      Start  8: test_contact_generator
 8/41 Test  #8: test_contact_generator ............   Passed    0.01 sec
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

### Demo Surrogate Recommendation: PASS

```bash
'<build>/Release/adasdf_recommend_demo.exe' --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
```

```text
AdaSDF-CL demo surrogate recommender
Status: experimental demo only
Warning: experimental demo surrogate; not universal; not fully trained; not an optimality guarantee
Surrogate: demo_v0_9
Shape: box
Requested top-k: 5

Candidate[0]
  target error: 0.001
  predicted p95 error: 0.001
  predicted memory: 0.492308 MB
  confidence: 0.8
  max octree level: 5
  block resolution: 32
  rank: 8
  compression: demo_low_rank
  warning: experimental demo surrogate; not universal; not fully trained; not an optimality guarantee

...
```

### Build Demo Adaptive SDFBin: PASS

```bash
'<build>/Release/adasdf_build_demo_adaptive.exe' '<workspace>/build/cube_adaptive_v1.sdfbin' --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
```

```text
AdaSDF-CL demo adaptive builder
Shape: box
Center: 0 0 0
Half extent: 0.5 0.5 0.5
Target near-surface error: 0.001
Memory limit: 64 MB
Block memory limit: 16 MB
Surrogate: demo_v0_9 experimental
Warning: experimental demo surrogate; not universal; not fully trained; not an optimality guarantee
Octree nodes: 6
Blocks: 7
Ranks: 8 7 6 8 7 6 8
Output: <local-path>
Reload validation: success
```

### Demo Adaptive Info CLI: PASS

```bash
'<build>/Release/adasdf_info.exe' '<workspace>/build/cube_adaptive_v1.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.0.2-alpha
Path: <local-path>
Model name: demo adaptive analytic box
Valid: yes
Format: ADASDF_DEMO_ADAPTIVE_SDFBIN_V1
Shape: box
Center: 0 0 0
Half extent: 0.5 0.5 0.5
Unit: m
Target near-surface error: 0.001
Memory limit MB: 64
Block expand limit MB: 16
Surrogate: demo_v0_9
Warning: demo_surrogate_not_universal_not_fully_trained
Demo octree nodes: 6
Demo adaptive blocks: 7
Block[0] resolution: 32
...
```

### Demo Adaptive Query CLI: PASS

```bash
'<build>/Release/adasdf_query.exe' '<workspace>/build/cube_adaptive_v1.sdfbin' --point 0 0 0
```

```text
AdaSDF-CL point query
Path: <local-path>
Point: 0 0 0
Signed distance: -0.5
Gradient: 1 0 0
Normal: 1 0 0
Query backend: core-free demo adaptive analytic SDF backend
```

### Demo Collide Boxes CLI: PASS

```bash
'<build>/Release/adasdf_collide_boxes_demo.exe' --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view '<workspace>/build/collision_v1.svg'
```

```text
AdaSDF-CL two-box demo collision
Surrogate: demo_v0_9 experimental demo only
Warning: experimental demo surrogate; not universal; not fully trained; not an optimality guarantee
Offset B: 0.25 0 0
Colliding: true
Minimum distance: -0.25
Backend: CPU narrow-phase: symmetric SDF-sampling research preview
Method: symmetric candidate-point SDF query + deterministic contact reduction
Requested max contacts: 8
Returned contacts: 8
Contact[0]
  point: 0 0 0
  normal: -1 0 0
  penetration_depth: 0.25
Contact[1]
  point: 0.5 -0.214286 -0.214286
  normal: -1 -0 -0
  penetration_depth: 0.25
...
```

### Batch Query Benchmark: PASS (CUDA SKIPPED)

```bash
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --points 10000,100000,1000000 --query-backend cpu,cuda --out '<workspace>/build/adasdf_batch_benchmark_v1.csv'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,phi_only,reuse_resident,kernel_only,status,error_message
cpu,none,all,10000,0,0,0,0,0,0.0889,0,NA,NA,0,0,0,0.469,NA,0.469,46.9,21321961.62,0,0,0,false,0,1,NA,NA,NA,NA,0.469,0.469,0.469,0,false,false,false,ok,
cpu,none,all,100000,0,0,0,0,0,0.77,0,NA,NA,0,0,0,4.7405,NA,4.7405,47.405,21094821.22,0,0,0,false,0,1,NA,NA,NA,NA,4.7405,4.7405,4.7405,0,false,false,false,ok,
cpu,none,all,1000000,0,0,0,0,0,7.4975,0,NA,NA,0,0,0,47.0205,NA,47.0205,47.0205,21267319.57,0,0,0,false,0,1,NA,NA,NA,NA,47.0205,47.0205,47.0205,0,false,false,false,ok,
cuda,global,all,10000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,0,1,NA,NA,NA,NA,,,,,false,false,false,skipped,CUDA backend unavailable
cuda,global,all,100000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,0,1,NA,NA,NA,NA,,,,,false,false,false,skipped,CUDA backend unavailable
cuda,global,all,1000000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,0,1,NA,NA,NA,NA,,,,,false,false,false,skipped,CUDA backend unavailable
backend | expansion | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | all | 10000 | 0 | 0.469 | NA | 46.9 | 0 | 0 | ok
cpu | none | all | 100000 | 0 | 4.7405 | NA | 47.405 | 0 | 0 | ok
cpu | none | all | 1000000 | 0 | 47.0205 | NA | 47.0205 | 0 | 0 | ok
cuda | global | all | 10000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
cuda | global | all | 100000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
cuda | global | all | 1000000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
```

### Collision SVG Check: PASS

```bash
check-collision-svg '<workspace>/build/collision_v1.svg'
```

```text
collision SVG generated and contains contact markers.
```

### Clean Check: PASS

```bash
'<local-path>' '<source>/scripts/check_repo_clean.py' '<source>'
```

```text
Repo clean check: PASS
```
