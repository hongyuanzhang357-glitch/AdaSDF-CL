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
--   CUDA backend: ON
--   CUDA toolkit: found
--   FCL: not required
--
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
  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_benchmark_batch_query.vcxproj -> <build>\benchmarks\Release\adasdf_benchmark_batch_query.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_build.vcxproj -> <build>\Release\adasdf_build.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_build_demo_adaptive.vcxproj -> <build>\Release\adasdf_build_demo_adaptive.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_build_then_query.vcxproj -> <build>\Release\adasdf_build_then_query.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_capabilities.vcxproj -> <build>\tools\Release\adasdf_capabilities.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_capability_walkthrough.vcxproj -> <build>\Release\adasdf_capability_walkthrough.exe
  Building Custom Rule <source>/CMakeLists.txt
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/54 Test  #1: test_sdf_io ...........................   Passed    0.03 sec
      Start  2: test_collision_query
 2/54 Test  #2: test_collision_query ..................   Passed    0.02 sec
      Start  3: test_distance_query
 3/54 Test  #3: test_distance_query ...................   Passed    0.01 sec
      Start  4: test_collision_object
 4/54 Test  #4: test_collision_object .................   Passed    0.01 sec
      Start  5: test_pair_distance_query
 5/54 Test  #5: test_pair_distance_query ..............   Passed    0.01 sec
      Start  6: test_pair_collision_query
 6/54 Test  #6: test_pair_collision_query .............   Passed    0.01 sec
      Start  7: test_candidate_point_sampler
 7/54 Test  #7: test_candidate_point_sampler ..........   Passed    0.01 sec
      Start  8: test_contact_generator
 8/54 Test  #8: test_contact_generator ................   Passed    0.02 sec
      Start  9: test_contact_reducer
...
```

### Install Validation: PASS

```bash
'<local-path>' '<source>/scripts/run_install_validation.py' --source '<source>' --build '<local-path>' --install '<local-path>' --config Release
```

```text
[install-validation] Configure
[install-validation] Build
[install-validation] Install
[install-validation] Package Configure
[install-validation] Package Build
[install-validation] Installed Capabilities CLI
[install-validation] Package Run
[install-validation] Downstream Configure
[install-validation] Downstream Build
[install-validation] Downstream Run
Install validation: PASS
Report: reports/install_validation_summary.md
```

### Capabilities CLI: PASS

```bash
'<build>/tools/Release/adasdf_capabilities.exe' --verbose
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
...
```

### Capability Walkthrough Example: PASS

```bash
'<build>/Release/adasdf_capability_walkthrough.exe'
```

```text
AdaSDF-CL capability walkthrough
Version: 1.1.1-alpha
Query backends: CPU direct, CPU expanded, optional CUDA expanded
Point phi: -0.5
Point normal: 1 0 0
Collision hit: true
Minimum distance: -0.25
Raw contacts: 420
Reduced contacts: 4
Contact point: 0 0 0
Contact normal: -1 0 0
Contact penetration depth: 0.25
Contact signed distance: -0.25
Expansion quality samples: 512
Expansion p95 abs error: 0.000217442
Sign mismatch rate: 0
Near-surface sign mismatch rate: 0
Status: ok
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
'<build>/Release/adasdf_build_demo_adaptive.exe' '<local-path>' --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
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
'<build>/Release/adasdf_info.exe' '<local-path>'
```

```text
AdaSDF-CL info
Library version: 1.1.1-alpha
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
'<build>/Release/adasdf_query.exe' '<local-path>' --point 0 0 0
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

### Expansion Quality CLI: PASS

```bash
'<build>/tools/Release/adasdf_expansion_quality.exe' '<local-path>' --expansion global --global-resolution 32 --samples 1000 --out '<local-path>'
```

```text
AdaSDF-CL expansion quality audit
Model: <local-path>
Expansion: global
Blocks: all
Resolution: 32
Samples: 1000
Finite samples: 1000
Max abs error: 0.00709197
Mean abs error: 0.000112949
RMS error: 0.00065218
P95 abs error: 0.000222009
Sign mismatch count: 0
Sign mismatch rate: 0
Ambiguous sign count: 256
Ambiguous sign rate: 0.256
Near-surface samples: 501
Near-surface sign mismatch count: 0
Near-surface sign mismatch rate: 0
...
```

### Demo Collide Boxes CLI: PASS

```bash
'<build>/Release/adasdf_collide_boxes_demo.exe' --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view '<local-path>'
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

### Batch Query Benchmark: PASS

```bash
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --points 10000,100000,1000000 --query-backend cpu,cuda --out '<local-path>'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,10000,0,0,0,0,0,0.1349,0,NA,NA,0,0,0,0.5488,NA,0.5488,54.88,18221574.34,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,0.5488,0.5488,0.5488,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,100000,0,0,0,0,0,0.848,0,NA,NA,0,0,0,5.1503,NA,5.1503,51.503,19416344.68,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,5.1503,5.1503,5.1503,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,1000000,0,0,0,0,0,7.7015,0,NA,NA,0,0,0,49.8246,NA,49.8246,49.8246,20070406.99,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,49.8246,49.8246,49.8246,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cuda,global,all,10000,2.000171661,2.000076294,43.9941,4.5551,39.4389,0.1146,0.0779,0.8025280237,0.1167,0.1355,0.0433,0.0078,1.2159,0.8025280237,1.2159,121.59,8224360.556,0,0.004385174555,0.8738651282,true,0.004385174555,6.99335381e-05,0.000394608336,1.526556659e-16,0,0,0,0,0,0,0,0,1,0.8025280237,0.8025280237,0.8025280237,0,1.2159,1.2159,1.2159,0,"phi,normal",false,false,false,false,1,10000,0.5340576172,10000,0,1,1,true,true,paged,aos,ok,
cuda,global,all,100000,2.000171661,2.000076294,5.8387,4.4806,1.3581,0.9563,0.6454,1.104096055,1.0971,0.9149,0.4393,0.492,4.7258,1.104096055,4.7258,47.258,21160438.44,0,0.005406495077,0.8738651282,true,0.005406495077,6.924660749e-05,0.0003859996001,2.247274656e-05,0,0,0,0,0,0,0,0,1,1.104096055,1.104096055,1.104096055,0,4.7258,4.7258,4.7258,0,"phi,normal",false,false,false,false,1,100000,5.340576172,100000,0,1,1,true,true,paged,aos,ok,
cuda,global,all,1000000,2.000171661,2.000076294,6.0911,4.7971,1.2938,7.6311,6.618,10.34895992,10.4413,6.8436,4.409,0.7314,37.9809,10.34895992,37.9809,37.9809,26329023.27,0,0.006410162008,0.8996876832,true,0.006410162008,7.064771896e-05,0.0003900821945,4.831315872e-05,0,0,0,0,0,0,0,0,1,10.34895992,10.34895992,10.34895992,0,37.9809,37.9809,37.9809,0,"phi,normal",false,false,false,false,1,1000000,53.40576172,1000000,0,1,1,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 10000 | 0 | 0.5488 | NA | 54.88 | 0 | 0 | ok
cpu | none | phi,normal | all | 100000 | 0 | 5.1503 | NA | 51.503 | 0 | 0 | ok
cpu | none | phi,normal | all | 1000000 | 0 | 49.8246 | NA | 49.8246 | 0 | 0 | ok
cuda | global | phi,normal | all | 10000 | 43.9941 | 1.2159 | 0.802528 | 121.59 | 0.00438517 | 0.873865 | ok
cuda | global | phi,normal | all | 100000 | 5.8387 | 4.7258 | 1.1041 | 47.258 | 0.0054065 | 0.873865 | ok
cuda | global | phi,normal | all | 1000000 | 6.0911 | 37.9809 | 10.349 | 37.9809 | 0.00641016 | 0.899688 | ok
```

### Collision SVG Check: PASS

```bash
check-collision-svg '<local-path>'
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
