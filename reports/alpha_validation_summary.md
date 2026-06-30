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
CMake Warning at CMakeLists.txt:72 (message):
  ADASDF_CL_ENABLE_CUDA=ON but no CUDA compiler was found.  Building without
  CUDA batch query kernels.


--
-- AdaSDF-CL configuration:
--   Version: 1.1.0-alpha
--   Build examples: ON
--   Build tests: ON
--   Benchmarks: ON
--   Existing core requested: OFF
--   Existing core found: OFF
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   Demo backend: ON
--   Demo surrogate: ON
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
  adasdf_collide.vcxproj -> <build>\Release\adasdf_collide.exe
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_collide_boxes_demo.vcxproj -> <build>\Release\adasdf_collide_boxes_demo.exe
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
 1/51 Test  #1: test_sdf_io ...........................   Passed    0.01 sec
      Start  2: test_collision_query
 2/51 Test  #2: test_collision_query ..................   Passed    0.01 sec
      Start  3: test_distance_query
 3/51 Test  #3: test_distance_query ...................   Passed    0.01 sec
      Start  4: test_collision_object
 4/51 Test  #4: test_collision_object .................   Passed    0.01 sec
      Start  5: test_pair_distance_query
 5/51 Test  #5: test_pair_distance_query ..............   Passed    0.01 sec
      Start  6: test_pair_collision_query
 6/51 Test  #6: test_pair_collision_query .............   Passed    0.01 sec
      Start  7: test_candidate_point_sampler
 7/51 Test  #7: test_candidate_point_sampler ..........   Passed    0.02 sec
      Start  8: test_contact_generator
 8/51 Test  #8: test_contact_generator ................   Passed    0.03 sec
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
Library version: 1.1.0-alpha
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

### Expansion Quality CLI: PASS

```bash
'<build>/tools/Release/adasdf_expansion_quality.exe' '<workspace>/build/cube_adaptive_v1.sdfbin' --expansion global --global-resolution 32 --samples 1000 --out '<workspace>/build/adasdf_expansion_quality_v1.csv'
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
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,10000,0,0,0,0,0,0.0987,0,NA,NA,0,0,0,0.5077,NA,0.5077,50.77,19696671.26,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,0.5077,0.5077,0.5077,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,100000,0,0,0,0,0,0.861,0,NA,NA,0,0,0,5.3507,NA,5.3507,53.507,18689143.48,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,5.3507,5.3507,5.3507,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,1000000,0,0,0,0,0,7.875,0,NA,NA,0,0,0,49.5954,NA,49.5954,49.5954,20163160.29,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,49.5954,49.5954,49.5954,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cuda,global,all,10000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
cuda,global,all,100000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
cuda,global,all,1000000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 10000 | 0 | 0.5077 | NA | 50.77 | 0 | 0 | ok
cpu | none | phi,normal | all | 100000 | 0 | 5.3507 | NA | 53.507 | 0 | 0 | ok
cpu | none | phi,normal | all | 1000000 | 0 | 49.5954 | NA | 49.5954 | 0 | 0 | ok
cuda | global | phi,normal | all | 10000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
cuda | global | phi,normal | all | 100000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
cuda | global | phi,normal | all | 1000000 | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | skipped
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
