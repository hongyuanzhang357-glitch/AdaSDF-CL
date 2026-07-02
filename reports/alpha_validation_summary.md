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
--   Version: 1.8.1-alpha
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
cmake --build '<build>' --config Release -- /nodeReuse:false
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  adasdf_adaptive_block_compression_demo.vcxproj -> <build>\Release\adasdf_adaptive_block_compression_demo.exe
  adasdf_adaptive_builder_preview_demo.vcxproj -> <build>\Release\adasdf_adaptive_builder_preview_demo.exe
  adasdf_adaptive_vs_dense_sdf_demo.vcxproj -> <build>\Release\adasdf_adaptive_vs_dense_sdf_demo.exe
  adasdf_benchmark_batch_query.vcxproj -> <build>\benchmarks\Release\adasdf_benchmark_batch_query.exe
  adasdf_build.vcxproj -> <build>\Release\adasdf_build.exe
  adasdf_build_adaptive_sdf.vcxproj -> <build>\tools\Release\adasdf_build_adaptive_sdf.exe
  adasdf_build_adaptive_sdf_preview.vcxproj -> <build>\tools\Release\adasdf_build_adaptive_sdf_preview.exe
  adasdf_build_compressed_sdf.vcxproj -> <build>\tools\Release\adasdf_build_compressed_sdf.exe
  adasdf_build_demo_adaptive.vcxproj -> <build>\Release\adasdf_build_demo_adaptive.exe
  adasdf_build_dense_sdf.vcxproj -> <build>\tools\Release\adasdf_build_dense_sdf.exe
  adasdf_build_recommend_demo.vcxproj -> <build>\Release\adasdf_build_recommend_demo.exe
  adasdf_build_then_query.vcxproj -> <build>\Release\adasdf_build_then_query.exe
  adasdf_capabilities.vcxproj -> <build>\tools\Release\adasdf_capabilities.exe
  adasdf_capability_walkthrough.vcxproj -> <build>\Release\adasdf_capability_walkthrough.exe
  adasdf_collide.vcxproj -> <build>\tools\Release\adasdf_collide.exe
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
        Start   1: test_sdf_io
  1/101 Test   #1: test_sdf_io ................................   Passed    0.01 sec
        Start   2: test_collision_query
  2/101 Test   #2: test_collision_query .......................   Passed    0.01 sec
        Start   3: test_distance_query
  3/101 Test   #3: test_distance_query ........................   Passed    0.01 sec
        Start   4: test_collision_object
  4/101 Test   #4: test_collision_object ......................   Passed    0.01 sec
        Start   5: test_pair_distance_query
  5/101 Test   #5: test_pair_distance_query ...................   Passed    0.01 sec
        Start   6: test_pair_collision_query
  6/101 Test   #6: test_pair_collision_query ..................   Passed    0.02 sec
        Start   7: test_candidate_point_sampler
  7/101 Test   #7: test_candidate_point_sampler ...............   Passed    0.01 sec
        Start   8: test_contact_generator
  8/101 Test   #8: test_contact_generator .....................   Passed    0.01 sec
        Start   9: test_contact_reducer
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
[install-validation] Python CLI Wrapper Tests
[install-validation] Installed Capabilities CLI
[install-validation] Installed Mesh Check CLI
[install-validation] Installed Mesh Clean CLI
[install-validation] Installed Build Recommendation CLI
[install-validation] Installed DenseSDF Build CLI
[install-validation] Installed DenseSDF Info CLI
[install-validation] Installed DenseSDF Query CLI
[install-validation] Installed DenseSDF Collide CLI
[install-validation] Installed AdaptiveBlockSDF Build CLI
[install-validation] Installed AdaptiveBlockSDF Info CLI
[install-validation] Installed AdaptiveBlockSDF Query CLI
[install-validation] Installed AdaptiveBlockSDF Collide CLI
...
```

### Python CLI Wrapper Tests: PASS

```bash
'<local-path>' -m unittest discover -s '<source>/python/tests'
```

```text
......................
----------------------------------------------------------------------
Ran 22 tests in 0.253s

OK
```

### Capabilities CLI: PASS

```bash
'<build>/tools/Release/adasdf_capabilities.exe' --verbose
```

```text
AdaSDF-CL version: 1.8.1-alpha
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
...
```

### Capability Walkthrough Example: PASS

```bash
'<build>/Release/adasdf_capability_walkthrough.exe'
```

```text
AdaSDF-CL capability walkthrough
Version: 1.8.1-alpha
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

### Mesh Diagnostics CLI: PASS

```bash
'<build>/tools/Release/adasdf_mesh_check.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --readiness --out '<workspace>/build/closed_cube_mesh_report.md' --json '<workspace>/build/closed_cube_mesh_report.json'
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
...
```

### Mesh Cleanup CLI: PASS

```bash
'<build>/tools/Release/adasdf_mesh_clean.exe' '<source>/tests/data/mesh_diagnostics/duplicate_and_degenerate_ascii.stl' '<workspace>/build/cleaned_duplicate_and_degenerate.stl' --report '<workspace>/build/mesh_cleanup_report.md'
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

### Cleaned Mesh Check CLI: PASS

```bash
'<build>/tools/Release/adasdf_mesh_check.exe' '<workspace>/build/cleaned_duplicate_and_degenerate.stl' --readiness --out '<workspace>/build/cleaned_mesh_report.md'
```

```text
AdaSDF-CL mesh diagnostics
Input: <local-path>
Format: ascii
Vertices: 3
Triangles: 1
Raw triangles: 1
AABB min: 0 0 0
AABB max: 1 1 0
Diagonal: 1.41421
Watertight: no
Boundary edges: 3
Non-manifold edges: 0
Degenerate triangles: 0
Duplicate triangles: 0
Connected components: 1
Isolated vertices: 0
Recommendation: Review warnings before SDF construction; no automatic repair was performed.
SDF build readiness: Poor
...
```

### Build Recommendation CLI: PASS

```bash
'<build>/tools/Release/adasdf_recommend_build.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --target-error 1e-3 --memory-mb 256 --use-case contact --out '<workspace>/build/closed_cube_recommendation.md' --json '<workspace>/build/closed_cube_recommendation.json' --emit-command
```

```text
AdaSDF-CL build recommender
Input: <local-path>
Target near-surface error: 0.001
Memory budget MB: 256
Use case: contact
Recommended path: AdaptiveBlockSDF
Confidence: high
Estimated memory MB: 5.76123
Estimated near-surface error: 0.000845728
Estimated compression ratio: 1
CLI command: adasdf_build_adaptive_sdf <local-path> closed_cube_ascii_adaptive.sdfbin --target-error 0.001 --min-level 3 --max-level 7 --block-resolution 12 --padding 0.05 --signed --report closed_cube_ascii_adaptive_report.md
Build executed: no
Model boundary: experimental deterministic recommender; not a universal trained model; not fully trained; not an optimality guarantee
Markdown report: <local-path>
JSON report: <local-path>
adasdf_build_adaptive_sdf <local-path> closed_cube_ascii_adaptive.sdfbin --target-error 0.001 --min-level 3 --max-level 7 --block-resolution 12 --padding 0.05 --signed --report closed_cube_ascii_adaptive_report.md
```

### DenseSDF Build CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_dense_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/closed_cube_dense_v1_5.sdfbin' --resolution 24 --padding 0.05 --report '<workspace>/build/closed_cube_dense_report.md' --json '<workspace>/build/closed_cube_dense_report.json'
```

```text
AdaSDF-CL dense SDF builder
Input: <local-path>
Output: <local-path>
Resolution: 24 x 24 x 24
Signed: yes
Watertight: yes
Triangles: 12
Build time ms: 22.7259
Memory bytes: 111128
Reload validation: success
Report: <local-path>
JSON report: <local-path>
```

### DenseSDF Info CLI: PASS

```bash
'<build>/tools/Release/adasdf_info.exe' '<workspace>/build/closed_cube_dense_v1_5.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.8.1-alpha
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
...
```

### DenseSDF Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_query.exe' '<workspace>/build/closed_cube_dense_v1_5.sdfbin' --point 0.5 0.5 0.5
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

### DenseSDF Collide CLI: PASS

```bash
'<build>/tools/Release/adasdf_collide.exe' '<workspace>/build/closed_cube_dense_v1_5.sdfbin' '<workspace>/build/closed_cube_dense_v1_5.sdfbin' --max-contacts 4
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
...
```

### AdaptiveBlockSDF Build CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' --max-level 2 --block-resolution 5 --report '<workspace>/build/closed_cube_adaptive_block_report.md' --json '<workspace>/build/closed_cube_adaptive_block_report.json'
```

```text
AdaSDF-CL adaptive block SDF builder
Input: <local-path>
Output: <local-path>
Signed: yes
Watertight: yes
Octree levels: 1-2
Max level used: 2
Block resolution: 5
Target near-surface error: 0.001
Octree nodes: 73
Leaf blocks: 64
Near-surface blocks: 64
Memory bytes: 75248
Sampling time ms: 12.4655
Build time ms: 13.7179
Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
Reload validation: success
Low-rank compression: not enabled
...
```

### AdaptiveBlockSDF Info CLI: PASS

```bash
'<build>/tools/Release/adasdf_info.exe' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.8.1-alpha
Path: <local-path>
Model name: adaptive block dense SDF
Valid: yes
Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
AdaptiveBlockSDF block_count: 64
AdaptiveBlockSDF near_surface_block_count: 64
AdaptiveBlockSDF max_level_used: 2
AdaptiveBlockSDF block_resolution: 5
AdaptiveBlockSDF signed: yes
AdaptiveBlockSDF storage: block-wise dense phi values
Low-rank compression: available in v1.7.0-alpha via compressed block workflow
Format version: 1
Query backend: core-free adaptive block dense SDF backend
Query backend available: yes
AABB min: -0.05 -0.05 -0.05
AABB max: 1.05 1.05 1.05
...
```

### AdaptiveBlockSDF Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_query.exe' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' --point 0.5 0.5 0.5
```

```text
AdaSDF-CL point query
Path: <local-path>
Point: 0.5 0.5 0.5
Signed distance: -0.5
Gradient: 8.07435e-16 8.07435e-16 8.07435e-16
Normal: 0 0 0
Query backend: core-free adaptive block dense SDF backend
```

### AdaptiveBlockSDF Collide CLI: PASS

```bash
'<build>/tools/Release/adasdf_collide.exe' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' --max-contacts 4
```

```text
AdaSDF-CL collision query
Model A: <local-path>
Model B: <local-path>
Offset B: 0 0 0
AABB A: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
AABB B: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
Colliding: true
Minimum distance: -0.5
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
...
```

### AdaptiveBlockSDF Expansion Quality CLI: PASS

```bash
'<build>/tools/Release/adasdf_expansion_quality.exe' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' --expansion global --global-resolution 16 --samples 200 --out '<workspace>/build/closed_cube_adaptive_block_quality.csv'
```

```text
AdaSDF-CL expansion quality audit
Model: <local-path>
Expansion: global
Blocks: all
Resolution: 16
Samples: 200
Finite samples: 200
Max abs error: 0.0196008
Mean abs error: 0.000980876
RMS error: 0.00270995
P95 abs error: 0.00474094
Sign mismatch count: 0
Sign mismatch rate: 0
Ambiguous sign count: 0
Ambiguous sign rate: 0
Near-surface samples: 0
Near-surface sign mismatch count: 0
Near-surface sign mismatch rate: 0
...
```

### AdaptiveBlockSDF Benchmark Model CLI: PASS

```bash
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --model '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' --points 1000 --query-backend cpu --expansion none --out '<workspace>/build/closed_cube_adaptive_block_benchmark.csv'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,1000,0,0,0,0,0,0.0173,0,NA,NA,0,0,0,11.2955,NA,11.2955,11295.5,88530.83086,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,11.2955,11.2955,11.2955,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 1000 | 0 | 11.2955 | NA | 11295.5 | 0 | 0 | ok
```

### CompressedSDF Compress CLI: PASS

```bash
'<build>/tools/Release/adasdf_compress_adaptive_sdf.exe' '<workspace>/build/closed_cube_adaptive_block_v1_7.sdfbin' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' --target-error 1e-3 --max-rank 5 --report '<workspace>/build/closed_cube_compression_report.md' --json '<workspace>/build/closed_cube_compression_report.json' --quality-report '<workspace>/build/closed_cube_compression_quality.md'
```

```text
AdaSDF-CL adaptive block SDF compressor
Input: <local-path>
Output: <local-path>
Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
Matrix-SVD blocks: 64
Dense fallback blocks: 0
Original memory bytes: 74840
Compressed memory bytes: 70232
Compression ratio: 0.911266
Max abs error: 4.996e-16
RMS error: 6.11371e-17
P95 abs error: 1.38778e-16
Quality samples: 4096
Reload validation: success
Tucker/HOSVD compression: planned / not implemented
GPU-native compressed query: planned
Report: <local-path>
JSON report: <local-path>
...
```

### CompressedSDF Info CLI: PASS

```bash
'<build>/tools/Release/adasdf_info.exe' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.8.1-alpha
Path: <local-path>
Model name: compressed adaptive block SDF
Valid: yes
Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
CompressedBlockSDF block_count: 64
CompressedBlockSDF matrix_svd_block_count: 64
CompressedBlockSDF dense_fallback_block_count: 0
CompressedBlockSDF near_surface_block_count: 64
CompressedBlockSDF max_level_used: 2
CompressedBlockSDF signed: yes
CompressedBlockSDF original_memory_bytes: 64000
CompressedBlockSDF compressed_memory_bytes: 70232
CompressedBlockSDF compression_ratio: 0.911266
CompressedBlockSDF rank_min: 1
CompressedBlockSDF rank_max: 5
CompressedBlockSDF max_abs_error: 4.996e-16
...
```

### CompressedSDF Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_query.exe' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' --point 0.5 0.5 0.5
```

```text
AdaSDF-CL point query
Path: <local-path>
Point: 0.5 0.5 0.5
Signed distance: -0.5
Gradient: 2.4223e-15 4.84461e-15 6.45948e-15
Normal: 0 0 0
Query backend: core-free compressed adaptive block SDF backend
```

### CompressedSDF Collide CLI: PASS

```bash
'<build>/tools/Release/adasdf_collide.exe' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' --max-contacts 4
```

```text
AdaSDF-CL collision query
Model A: <local-path>
Model B: <local-path>
Offset B: 0 0 0
AABB A: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
AABB B: min=(-0.05 -0.05 -0.05) max=(1.05 1.05 1.05)
Colliding: true
Minimum distance: -0.5
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
...
```

### CompressedSDF Expansion Quality CLI: PASS

```bash
'<build>/tools/Release/adasdf_expansion_quality.exe' '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' --expansion global --global-resolution 16 --samples 200 --out '<workspace>/build/closed_cube_compressed_quality.csv'
```

```text
AdaSDF-CL expansion quality audit
Model: <local-path>
Expansion: global
Blocks: all
Resolution: 16
Samples: 200
Finite samples: 200
Max abs error: 0.0196008
Mean abs error: 0.000980876
RMS error: 0.00270995
P95 abs error: 0.00474094
Sign mismatch count: 0
Sign mismatch rate: 0
Ambiguous sign count: 0
Ambiguous sign rate: 0
Near-surface samples: 0
Near-surface sign mismatch count: 0
Near-surface sign mismatch rate: 0
...
```

### CompressedSDF Benchmark Model CLI: PASS

```bash
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --model '<workspace>/build/closed_cube_compressed_block_v1_7.sdfbin' --points 1000 --query-backend cpu --expansion none --out '<workspace>/build/closed_cube_compressed_benchmark.csv'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,1000,0,0,0,0,0,0.0153,0,NA,NA,0,0,0,1.9328,NA,1.9328,1932.8,517384.106,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,1.9328,1.9328,1.9328,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 1000 | 0 | 1.9328 | NA | 1932.8 | 0 | 0 | ok
```

### CompressedSDF One-Step Build CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_compressed_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/closed_cube_compressed_direct_v1_7.sdfbin' --target-error 1e-3 --max-level 2 --block-resolution 5 --max-rank 5 --report '<workspace>/build/closed_cube_compressed_direct_report.md' --compression-report '<workspace>/build/closed_cube_compressed_direct_compression_report.md' --quality-report '<workspace>/build/closed_cube_compressed_direct_quality.md'
```

```text
AdaSDF-CL compressed SDF builder
Input: <local-path>
Output: <local-path>
Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
Adaptive blocks: 64
Matrix-SVD blocks: 64
Dense fallback blocks: 0
Compression ratio: 0.911266
Max abs error: 4.996e-16
Quality samples: 4096
Reload validation: success
Tucker/HOSVD compression: planned / not implemented
Surrogate recommendation: use adasdf_recommend_build (implemented in v1.8.0-alpha)
GPU-native compressed query: planned
Build report: <local-path>
Compression report: <local-path>
Quality report: <local-path>
```

### AdaptiveBlockSDF Dry Run CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/closed_cube_adaptive_dryrun.sdfbin' --max-level 2 --block-resolution 5 --dry-run --report '<workspace>/build/closed_cube_adaptive_dryrun_plan.md'
```

```text
AdaSDF-CL adaptive block SDF builder
Input: <local-path>
Requested output: <local-path>
Dry run: yes
Signed: yes
Octree levels: 1-2
Block resolution: 5
Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
Output written: no
Low-rank compression: not enabled
Report: <local-path>
```

### Adaptive Builder Preview CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_adaptive_sdf_preview.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/closed_cube_adaptive_preview.sdfbin' --target-error 1e-3 --memory-mb 512 --dry-run --plan '<workspace>/build/closed_cube_adaptive_preview_plan.md'
```

```text
AdaSDF-CL adaptive builder interface preview
Input: <local-path>
Requested output: <local-path>
Dry run: yes
Valid plan: yes
Implemented in this version: yes
Adaptive octree/block SDF construction is implemented in v1.6.0-alpha as block-wise dense output.
Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF construction.
Low-rank block compression is implemented in v1.7.0-alpha using matrix-SVD per adaptive block.
Use adasdf_build_compressed_sdf for one-step compressed output.
Surrogate-guided build recommendation is implemented in v1.8.0-alpha as an experimental deterministic estimator.
Use adasdf_recommend_build for parameter recommendation.
Tucker/HOSVD compression, trained surrogate models, and GPU-native compressed query remain planned.
Plan: <local-path>
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
'<build>/tools/Release/adasdf_info.exe' '<workspace>/build/cube_adaptive_v1.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.8.1-alpha
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
'<build>/tools/Release/adasdf_query.exe' '<workspace>/build/cube_adaptive_v1.sdfbin' --point 0 0 0
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
cpu,none,all,10000,0,0,0,0,0,0.0891,0,NA,NA,0,0,0,0.4626,NA,0.4626,46.26,21616947.69,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,0.4626,0.4626,0.4626,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,100000,0,0,0,0,0,0.9047,0,NA,NA,0,0,0,5.0618,NA,5.0618,50.618,19755818.09,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,5.0618,5.0618,5.0618,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,1000000,0,0,0,0,0,9.9711,0,NA,NA,0,0,0,51.3876,NA,51.3876,51.3876,19459947.54,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,51.3876,51.3876,51.3876,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cuda,global,all,10000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
cuda,global,all,100000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
cuda,global,all,1000000,,,,,,,,NA,NA,,,,,NA,,,,0,,NA,false,,,,,0,,0,,0,,,0,1,NA,NA,NA,NA,,,,,"phi,normal",false,false,false,false,0,0,,0,0,,,true,true,paged,aos,skipped,CUDA backend unavailable
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 10000 | 0 | 0.4626 | NA | 46.26 | 0 | 0 | ok
cpu | none | phi,normal | all | 100000 | 0 | 5.0618 | NA | 50.618 | 0 | 0 | ok
cpu | none | phi,normal | all | 1000000 | 0 | 51.3876 | NA | 51.3876 | 0 | 0 | ok
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
