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
-- Looking for a CUDA compiler - C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/bin/nvcc.exe
-- The CUDA compiler identification is NVIDIA 12.6.85 with host compiler MSVC 19.42.34444.0
-- Detecting CUDA compiler ABI info
-- Detecting CUDA compiler ABI info - done
-- Check for working CUDA compiler: C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/bin/nvcc.exe - skipped
-- Detecting CUDA compile features
-- Detecting CUDA compile features - done
--
-- AdaSDF-CL configuration:
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
  Compiling CUDA source file ..\sdf流程原理\OctreeBlockLowRankSDF\OctreeBlockLowRankSDF\adasdf_cl\src\backend\CudaQueryKernels.cu...

  <build>>"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\nvcc.exe"  --use-local-env -ccbin "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.42.34433\bin\HostX64\x64" -x cu   -I"<source>\include" -I"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\include" -I"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\include"     --keep-dir adasdf_c.4599A1BA\x64\Release  -maxrregcount=0   --machine 64 --compile -cudart static -forward-unknown-to-host-compiler -std=c++17 --generate-code=arch=compute_52,code=[compute_52,sm_52] -Xcompiler="/EHsc -Ob2"   -D_WINDOWS -DNDEBUG -DADASDF_CL_HAS_EXISTING_CORE=0 -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=1 -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=1 -DADASDF_CL_ENABLE_DEMO_BACKEND=1 -DADASDF_CL_ENABLE_DEMO_SURROGATE=1 -DADASDF_CL_ENABLE_COLLISION_VIEWER=1 -DADASDF_CL_HAS_CUDA_BACKEND=1 -DADASDF_CL_VERSION_MAJOR=1 -DADASDF_CL_VERSION_MINOR=0 -DADASDF_CL_VERSION_PATCH=1 -D"ADASDF_CL_VERSION_SUFFIX=\"alpha\"" -DADASDF_CL_ENABLE_CUDA=1 -D"CMAKE_INTDIR=\"Release\"" -D_MBCS -DWIN32 -D_WINDOWS -DNDEBUG -DADASDF_CL_HAS_EXISTING_CORE=0 -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=1 -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=1 -DADASDF_CL_ENABLE_DEMO_BACKEND=1 -DADASDF_CL_ENABLE_DEMO_SURROGATE=1 -DADASDF_CL_ENABLE_COLLISION_VIEWER=1 -DADASDF_CL_HAS_CUDA_BACKEND=1 -DADASDF_CL_VERSION_MAJOR=1 -DADASDF_CL_VERSION_MINOR=0 -DADASDF_CL_VERSION_PATCH=1 -D"ADASDF_CL_VERSION_SUFFIX=\"alpha\"" -DADASDF_CL_ENABLE_CUDA=1 -D"CMAKE_INTDIR=\"Release\"" -Xcompiler "/EHsc /W1 /nologo /O2 /FS   /MD " -Xcompiler "/Fd<build>\Release\adasdf_cl_runtime.pdb" -o adasdf_cl_runtime.dir\Release\CudaQueryKernels.obj "<source>\src\backend\CudaQueryKernels.cu"
  CudaQueryKernels.cu
  tmpxft_0000edfc_00000000-7_CudaQueryKernels.cudafe1.cpp
  FCLAdapter.cpp
  Backend.cpp
  CudaQueryBackend.cpp
  GpuBackend.cpp
  PointCloudGenerator.cpp
  CompressedSDF.cpp
  AdaptiveSDFBuilder.cpp
  DemoAdaptiveSDFBuilder.cpp
  DemoSurrogateRecommender.cpp
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
      Start  1: test_sdf_io
 1/37 Test  #1: test_sdf_io .......................   Passed    0.04 sec
      Start  2: test_collision_query
 2/37 Test  #2: test_collision_query ..............   Passed    0.02 sec
      Start  3: test_distance_query
 3/37 Test  #3: test_distance_query ...............   Passed    0.02 sec
      Start  4: test_collision_object
 4/37 Test  #4: test_collision_object .............   Passed    0.05 sec
      Start  5: test_pair_distance_query
 5/37 Test  #5: test_pair_distance_query ..........   Passed    0.02 sec
      Start  6: test_pair_collision_query
 6/37 Test  #6: test_pair_collision_query .........   Passed    0.02 sec
      Start  7: test_candidate_point_sampler
 7/37 Test  #7: test_candidate_point_sampler ......   Passed    0.01 sec
      Start  8: test_contact_generator
 8/37 Test  #8: test_contact_generator ............   Passed    0.02 sec
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
Library version: 1.0.1-alpha
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
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,status,error_message
cpu,none,all,10000,0,0,0.0001,NA,0.6188,61.88,16160310.28,0,0,0,true,ok,
cpu,none,all,100000,0,0,0.0008,NA,5.8669,58.669,17044776.63,0,0,0,true,ok,
cpu,none,all,1000000,0,0,0.0008,NA,59.4015,59.4015,16834591.72,0,0,0,true,ok,
cuda,global,all,10000,2.000148773,2.000076294,64.5041,1.24950397,1.8211,182.11,5491186.645,0,0.004385174555,0.8738651282,true,ok,
cuda,global,all,100000,2.000148773,2.000076294,6.2309,1.201408029,5.0511,50.511,19797667.83,0,0.005406495077,0.8738651282,true,ok,
cuda,global,all,1000000,2.000148773,2.000076294,5.8795,10.08463955,39.7572,39.7572,25152676.75,0,0.006410162008,0.8996876832,true,ok,
backend | expansion | blocks | points | setup ms | query total ms | kernel ms | ns/query | qps | fallback | max phi error | max normal error | status
cpu | none | all | 10000 | 0.0001 | 0.6188 | NA | 61.88 | 1.61603e+07 | 0 | 0 | 0 | ok
cpu | none | all | 100000 | 0.0008 | 5.8669 | NA | 58.669 | 1.70448e+07 | 0 | 0 | 0 | ok
cpu | none | all | 1000000 | 0.0008 | 59.4015 | NA | 59.4015 | 1.68346e+07 | 0 | 0 | 0 | ok
cuda | global | all | 10000 | 64.5041 | 1.8211 | 1.2495 | 182.11 | 5.49119e+06 | 0 | 0.00438517 | 0.873865 | ok
cuda | global | all | 100000 | 6.2309 | 5.0511 | 1.20141 | 50.511 | 1.97977e+07 | 0 | 0.0054065 | 0.873865 | ok
cuda | global | all | 1000000 | 5.8795 | 39.7572 | 10.0846 | 39.7572 | 2.51527e+07 | 0 | 0.00641016 | 0.899688 | ok
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
