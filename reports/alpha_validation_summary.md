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
--   Version: 1.15.0-alpha
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
cmake --build '<build>' --config Release -- /nodeReuse:false
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  adasdf_cl_runtime.vcxproj -> <build>\Release\adasdf_cl_runtime.lib
  adasdf_active_block_query.vcxproj -> <build>\tools\Release\adasdf_active_block_query.exe
  adasdf_adaptive_block_compression_demo.vcxproj -> <build>\Release\adasdf_adaptive_block_compression_demo.exe
  adasdf_adaptive_builder_preview_demo.vcxproj -> <build>\Release\adasdf_adaptive_builder_preview_demo.exe
  adasdf_adaptive_vs_dense_sdf_demo.vcxproj -> <build>\Release\adasdf_adaptive_vs_dense_sdf_demo.exe
  adasdf_benchmark_batch_query.vcxproj -> <build>\benchmarks\Release\adasdf_benchmark_batch_query.exe
  adasdf_benchmark_block_cache.vcxproj -> <build>\tools\Release\adasdf_benchmark_block_cache.exe
  adasdf_benchmark_builder_acceleration.vcxproj -> <build>\tools\Release\adasdf_benchmark_builder_acceleration.exe
  adasdf_benchmark_collision_world.vcxproj -> <build>\tools\Release\adasdf_benchmark_collision_world.exe
  adasdf_benchmark_contact_reduction.vcxproj -> <build>\tools\Release\adasdf_benchmark_contact_reduction.exe
  adasdf_benchmark_cuda_block_cache.vcxproj -> <build>\tools\Release\adasdf_benchmark_cuda_block_cache.exe
  adasdf_benchmark_sparse_query.vcxproj -> <build>\tools\Release\adasdf_benchmark_sparse_query.exe
  adasdf_build.vcxproj -> <build>\Release\adasdf_build.exe
  adasdf_build_adaptive_sdf.vcxproj -> <build>\tools\Release\adasdf_build_adaptive_sdf.exe
  adasdf_build_adaptive_sdf_preview.vcxproj -> <build>\tools\Release\adasdf_build_adaptive_sdf_preview.exe
  adasdf_build_compressed_sdf.vcxproj -> <build>\tools\Release\adasdf_build_compressed_sdf.exe
...
```

### CTest: PASS

```bash
ctest --test-dir '<build>' -C Release --output-on-failure
```

```text
Test project <build>
        Start   1: test_sdf_io
  1/175 Test   #1: test_sdf_io ................................   Passed    0.01 sec
        Start   2: test_collision_query
  2/175 Test   #2: test_collision_query .......................   Passed    0.01 sec
        Start   3: test_distance_query
  3/175 Test   #3: test_distance_query ........................   Passed    0.01 sec
        Start   4: test_collision_object
  4/175 Test   #4: test_collision_object ......................   Passed    0.01 sec
        Start   5: test_pair_distance_query
  5/175 Test   #5: test_pair_distance_query ...................   Passed    0.01 sec
        Start   6: test_pair_collision_query
  6/175 Test   #6: test_pair_collision_query ..................   Passed    0.01 sec
        Start   7: test_candidate_point_sampler
  7/175 Test   #7: test_candidate_point_sampler ...............   Passed    0.01 sec
        Start   8: test_contact_generator
  8/175 Test   #8: test_contact_generator .....................   Passed    0.01 sec
        Start   9: test_contact_reducer
...
```

### Install Validation: PASS

```bash
'<local-path>' '<source>/scripts/run_install_validation.py' --source '<source>' --build '<build>' --install '<local-path>' --config Release --reuse-build
```

```text
[install-validation] Reuse Existing Build
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
[install-validation] Installed CompressedSDF Compress CLI
...
```

### Python CLI Wrapper Tests: PASS

```bash
'<local-path>' -m unittest discover -s '<source>/python/tests'
```

```text
..................................................
----------------------------------------------------------------------
Ran 50 tests in 0.295s

OK
```

### Capabilities CLI: PASS

```bash
'<build>/tools/Release/adasdf_capabilities.exe' --verbose
```

```text
AdaSDF-CL version: 1.15.0-alpha
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
Version: 1.15.0-alpha
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
'<build>/tools/Release/adasdf_mesh_check.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --readiness --out '<local-path>' --json '<local-path>'
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
'<build>/tools/Release/adasdf_mesh_clean.exe' '<source>/tests/data/mesh_diagnostics/duplicate_and_degenerate_ascii.stl' '<local-path>' --report '<local-path>'
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
'<build>/tools/Release/adasdf_mesh_check.exe' '<local-path>' --readiness --out '<local-path>'
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
'<build>/tools/Release/adasdf_recommend_build.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --target-error 1e-3 --memory-mb 256 --use-case contact --out '<local-path>' --json '<local-path>' --emit-command
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
'<build>/tools/Release/adasdf_build_dense_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<local-path>' --resolution 24 --padding 0.05 --report '<local-path>' --json '<local-path>'
```

```text
AdaSDF-CL dense SDF builder
Input: <local-path>
Output: <local-path>
Resolution: 24 x 24 x 24
Signed: yes
Watertight: yes
Triangles: 12
Acceleration: brute
Used BVH: no
Threads requested: 1
Threads used: 1
BVH build time ms: 0
Sampling time ms: 21.623
BVH nodes: 0
BVH leaves: 0
Brute reference time ms: 0
Speedup vs brute reference: 0
Build time ms: 21.7209
...
```

### DenseSDF Info CLI: PASS

```bash
'<build>/tools/Release/adasdf_info.exe' '<local-path>'
```

```text
AdaSDF-CL info
Library version: 1.15.0-alpha
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
'<build>/tools/Release/adasdf_query.exe' '<local-path>' --point 0.5 0.5 0.5
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
'<build>/tools/Release/adasdf_collide.exe' '<local-path>' '<local-path>' --max-contacts 4
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
'<build>/tools/Release/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<local-path>' --max-level 2 --block-resolution 5 --report '<local-path>' --json '<local-path>'
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
Acceleration: brute
Used BVH: no
Threads requested: 1
Threads used: 1
BVH build time ms: 0
...
```

### AdaptiveBlockSDF Info CLI: PASS

```bash
'<build>/tools/Release/adasdf_info.exe' '<local-path>'
```

```text
AdaSDF-CL info
Library version: 1.15.0-alpha
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
'<build>/tools/Release/adasdf_query.exe' '<local-path>' --point 0.5 0.5 0.5
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
'<build>/tools/Release/adasdf_collide.exe' '<local-path>' '<local-path>' --max-contacts 4
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
'<build>/tools/Release/adasdf_expansion_quality.exe' '<local-path>' --expansion global --global-resolution 16 --samples 200 --out '<local-path>'
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
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --model '<local-path>' --points 1000 --query-backend cpu --expansion none --out '<local-path>'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,1000,0,0,0,0,0,0.0194,0,NA,NA,0,0,0,11.7109,NA,11.7109,11710.9,85390.53361,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,11.7109,11.7109,11.7109,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 1000 | 0 | 11.7109 | NA | 11710.9 | 0 | 0 | ok
```

### CompressedSDF Compress CLI: PASS

```bash
'<build>/tools/Release/adasdf_compress_adaptive_sdf.exe' '<local-path>' '<local-path>' --target-error 1e-3 --max-rank 5 --report '<local-path>' --json '<local-path>' --quality-report '<local-path>'
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
'<build>/tools/Release/adasdf_info.exe' '<local-path>'
```

```text
AdaSDF-CL info
Library version: 1.15.0-alpha
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
'<build>/tools/Release/adasdf_query.exe' '<local-path>' --point 0.5 0.5 0.5
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
'<build>/tools/Release/adasdf_collide.exe' '<local-path>' '<local-path>' --max-contacts 4
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
'<build>/tools/Release/adasdf_expansion_quality.exe' '<local-path>' --expansion global --global-resolution 16 --samples 200 --out '<local-path>'
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
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --model '<local-path>' --points 1000 --query-backend cpu --expansion none --out '<local-path>'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,1000,0,0,0,0,0,0.0149,0,NA,NA,0,0,0,1.9263,NA,1.9263,1926.3,519129.9382,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,1.9263,1.9263,1.9263,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 1000 | 0 | 1.9263 | NA | 1926.3 | 0 | 0 | ok
```

### CompressedSDF One-Step Build CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_compressed_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<local-path>' --target-error 1e-3 --max-level 2 --block-resolution 5 --max-rank 5 --report '<local-path>' --compression-report '<local-path>' --quality-report '<local-path>' --strict-json '<local-path>' --case-id alpha_compressed_direct
```

```text
AdaSDF-CL compressed SDF builder
Input: <local-path>
Output: <local-path>
Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
Adaptive blocks: 64
Acceleration: brute
Used BVH: no
Threads requested: 1
Threads used: 1
BVH build time ms: 0
Sampling time ms: 13.138
BVH nodes: 0
BVH leaves: 0
Brute reference time ms: 0
Speedup vs brute reference: 0
Matrix-SVD blocks: 64
Dense fallback blocks: 0
Compression ratio: 0.911266
...
```

### CollisionWorld Broadphase CLI: PASS

```bash
'<build>/tools/Release/adasdf_world_broadphase.exe' '<local-path>' --out '<local-path>' --report '<local-path>' --json '<local-path>'
```

```text
AdaSDF-CL CollisionWorld broadphase
Objects: 3
Tested pairs: 3
Overlap pairs: 1
Static-static skipped: 0
Group/mask skipped: 0
AABB rejected: 2
Status: ok
```

### CollisionWorld Sparse Collide CLI: PASS

```bash
'<build>/tools/Release/adasdf_world_sparse_collide.exe' '<local-path>' --threshold 0 --early-exit --out '<local-path>' --report '<local-path>' --json '<local-path>' --strict-json '<local-path>' --case-id alpha_world_sparse
```

```text
AdaSDF-CL CollisionWorld sparse collision
Mode: collision-only
Colliding: true
Broadphase pairs: 1
Queried pairs: 1
Queried samples: 1
Violations: 1
Min effective phi: -0.5
Sample-based SDF collision, not exact mesh-vs-mesh contact.
Return code note: 10 means collision detected, not failure
Status: ok

Validation note: world_sparse_collide returned 10, which means collision detected and is expected for this fixture.
```

### CollisionWorld Solver Contacts CLI: PASS

```bash
'<build>/tools/Release/adasdf_world_solver_contacts.exe' '<local-path>' --threshold 1e-3 --top-k 32 --max-contacts 8 --out '<local-path>' --report '<local-path>' --json '<local-path>'
```

```text
AdaSDF-CL CollisionWorld solver-ready contacts
Raw candidates: 4
Reduced candidates: 4
Solver contacts: 2
Patches: 2
Max penetration: 0.501
This exports solver-ready candidates, not solver constraints.
No impulses or friction forces are computed.
Status: ok
```

### CollisionWorld Benchmark: PASS

```bash
'<build>/tools/Release/adasdf_benchmark_collision_world.exe' '<local-path>' --mode sparse --threshold 1e-3 --repeat 2 --warmup 1 --csv '<local-path>' --report '<local-path>' --json '<local-path>' --strict-json '<local-path>' --case-id alpha_world_benchmark
```

```text
AdaSDF-CL CollisionWorld benchmark
mode: sparse
broadphase_pairs: 1
queried_pairs: 1
violations: 4
solver_contacts: 0
avg_total_ms: 0.01045
repeat: 2
warmup: 1
Status: ok
```

### Sparse Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_sparse_query.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 0 --out '<local-path>' --report '<local-path>' --strict-json '<local-path>' --case-id alpha_sparse_query
```

```text
AdaSDF-CL sparse SDF query
Sample count: 6
Queried count: 6
Result count: 6
Colliding: true
Min phi: -0.5
Min effective phi: -0.5
Early exit: false
Elapsed ms: 0.0086
Output mode: phi-only
Status: ok
```

### Sparse Collide CLI: PASS

```bash
'<build>/tools/Release/adasdf_sparse_collide.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 0 --early-exit --report '<local-path>'
```

```text
AdaSDF-CL sparse collision query
Mode: collision-only
Colliding: true
Min phi: -0.5
Min effective phi: -0.5
First hit sample id: 0
Sample count: 6
Queried samples: 1
Early exit: true
Elapsed ms: 0.0024
Return code note: 10 means collision detected, not failure
Status: ok

Validation note: sparse_collide returned 10, which means collision detected and is expected for this fixture.
```

### Contact Candidates CLI: PASS

```bash
'<build>/tools/Release/adasdf_contact_candidates.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --top-k 4 --threshold 1e-3 --reduction-radius 0.02 --with-normal --out '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL contact candidates
Input samples: 6
Threshold candidates: 2
Reduced candidates: 2
Top-K: 4
Min effective phi: -0.5
This is not a full contact manifold or solver constraints.
Status: ok
```

### Contact Stabilization CLI: PASS

```bash
'<build>/tools/Release/adasdf_stabilize_contacts.exe' '<local-path>' --max-contacts 8 --patch-radius 0.02 --out '<local-path>' --json '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL stabilize contacts
Input candidates: 2
Patches: 2
Output solver contacts: 2
Max contacts: 8
Warnings: 0
This exports solver-ready candidates, not solver constraints.
No impulses or friction forces are computed.
Status: ok
```

### Solver Contact Candidates CLI: PASS

```bash
'<build>/tools/Release/adasdf_solver_contact_candidates.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1e-3 --top-k 32 --max-contacts 8 --patch-radius 0.02 --with-normal --out '<local-path>' --candidates-out '<local-path>' --json '<local-path>' --report '<local-path>' --strict-json '<local-path>' --case-id alpha_solver_contacts
```

```text
AdaSDF-CL solver contact candidates
Samples: 6
Raw candidates: 2
Patches: 2
Solver contacts: 2
Max penetration: 0.501
Max contacts: 8
This exports solver-ready candidates, not solver constraints.
No impulses or friction forces are computed.
Status: ok
```

### Contact Reduction Benchmark: PASS

```bash
'<build>/tools/Release/adasdf_benchmark_contact_reduction.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1e-3 --top-k 64 --max-contacts 8 --patch-radius 0.02 --repeat 2 --warmup 1 --csv '<local-path>' --json '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL contact reduction benchmark
sample_count: 6
raw_candidate_count: 2
patch_count: 2
solver_contact_count: 2
candidate_reduction_ratio: 1
avg_query_ms: 0.0155
avg_reduction_ms: 0.00545
avg_total_ms: 0.02095
max_contacts: 8
patch_radius: 0.02
repeat: 2
warmup: 1
Status: ok
```

### Sparse Query Benchmark: PASS

```bash
'<build>/tools/Release/adasdf_benchmark_sparse_query.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 1 --mode phi-only --csv '<local-path>'
```

```text
sample_count,repeat,warmup,total_ms,avg_ms,avg_us,avg_ns_per_sample,queried_samples_avg,early_exit_rate,mode,with_normal,threshold,top_k,status
6,2,1,0.0082,0.0041,4.1,683.333,6,0,phi-only,false,0,8,ok
Sparse benchmark mode: phi-only
Average ns per sample: 683.333
Status: ok
```

### Active Block Selection CLI: PASS

```bash
'<build>/tools/Release/adasdf_select_active_blocks.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --out '<local-path>' --report '<local-path>' --json '<local-path>'
```

```text
AdaSDF-CL active block selection
Sample count: 6
Candidate samples: 6
Active blocks: 64
Threshold: 1
Selection band: 0.1
Extra margin: 0.02
Status: ok
```

### Active Block Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_active_block_query.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --with-normal --out '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL active block query
Sample count: 6
Queried count: 6
Result count: 6
Cache queries: 4
Fallback queries: 2
Resident blocks: 64
Resident memory bytes: 74752
Colliding: true
Min effective phi: -0.5
Query time ms: 0.1193
Status: ok

Validation note: active_block_query returned 10, which means collision detected and is expected for this fixture.
```

### Active Block Cache Benchmark: PASS

```bash
'<build>/tools/Release/adasdf_benchmark_block_cache.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 1 --threshold 1.0 --selection-band 0.1 --compare-direct --csv '<local-path>' --report '<local-path>'
```

```text
sample_count,repeat,warmup,active_block_count,expanded_block_count,cache_memory_bytes,cache_hit_rate,fallback_query_count,active_block_avg_ms,active_block_ns_per_sample,direct_avg_ms,direct_ns_per_sample,mode,threshold,selection_band,status
6,2,1,45,45,52560,1.000000,4,0.017700,2950.000000,0.004450,741.666667,phi-only,1.000000,0.100000,ok
Active block cache benchmark mode: phi-only
Average ns per sample: 2950
Status: ok
```

### CUDA Active Block Query CLI: PASS

```bash
'<build>/tools/Release/adasdf_cuda_active_block_query.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --with-normal --out '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL CUDA active block query
CUDA available: true
Sample count: 6
Queried count: 6
Result count: 6
Active blocks: 20
Expanded blocks: 20
GPU query count: 4
Fallback queries: 2
GPU memory bytes: 22560
Selection time ms: 0.0203
CPU expansion time ms: 0.0341
Upload time ms: 43.5989
Block upload time ms: 43.5731
Sample upload time ms: 0.0258
Kernel time ms: 0.652288
Download time ms: 0.0983
Total time ms: 64.2719
...
```

### CUDA Active Block Cache Benchmark: PASS

```bash
'<build>/tools/Release/adasdf_benchmark_cuda_block_cache.exe' '<local-path>' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 0 --threshold 1.0 --selection-band 0.1 --compare-direct --csv '<local-path>' --report '<local-path>'
```

```text
AdaSDF-CL CUDA active block cache benchmark
CUDA available: true
CUDA block cache benchmark mode: phi-only
Sample count: 6
Active blocks: 4
GPU memory bytes: 4512
CUDA total avg ms: 21.412
CUDA kernel avg ms: 0.456768
CUDA upload avg ms: 20.8102
CUDA download avg ms: 0.06835
Average ns per sample: 3.56867e+06
CPU active avg ms: 0.06855
Direct sparse avg ms: 0.00435
Status: ok
```

### AdaptiveBlockSDF Dry Run CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<local-path>' --max-level 2 --block-resolution 5 --dry-run --report '<local-path>'
```

```text
AdaSDF-CL adaptive block SDF builder
Input: <local-path>
Requested output: <local-path>
Dry run: yes
Signed: yes
Octree levels: 1-2
Block resolution: 5
Acceleration: brute
Threads requested: 1
Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
Output written: no
Low-rank compression: not enabled
Report: <local-path>
```

### Adaptive Builder Preview CLI: PASS

```bash
'<build>/tools/Release/adasdf_build_adaptive_sdf_preview.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<local-path>' --target-error 1e-3 --memory-mb 512 --dry-run --plan '<local-path>'
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
'<build>/tools/Release/adasdf_info.exe' '<local-path>'
```

```text
AdaSDF-CL info
Library version: 1.15.0-alpha
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
'<build>/tools/Release/adasdf_query.exe' '<local-path>' --point 0 0 0
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
'<build>/benchmarks/Release/adasdf_benchmark_batch_query.exe' --points 10000,100000,1000000 --query-backend cpu,cuda --out '<local-path>' --strict-json '<local-path>' --case-id alpha_batch_benchmark
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,10000,0,0,0,0,0,0.0959,0,NA,NA,0,0,0,0.5007,NA,0.5007,50.07,19972039.15,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,0.5007,0.5007,0.5007,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,100000,0,0,0,0,0,0.8615,0,NA,NA,0,0,0,5.0576,NA,5.0576,50.576,19772223.98,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,5.0576,5.0576,5.0576,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cpu,none,all,1000000,0,0,0,0,0,8.2515,0,NA,NA,0,0,0,50.7186,NA,50.7186,50.7186,19716632.56,0,0,0,true,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,50.7186,50.7186,50.7186,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
cuda,global,all,10000,2.000171661,2.000076294,46.3212,4.8118,41.5092,0.1171,0.0726,1.098431945,0.1168,0.1325,0.0434,0.0078,1.5054,1.098431945,1.5054,150.54,6642752.757,0,0.004385174555,0.8738651282,true,0.004385174555,6.99335381e-05,0.000394608336,1.526556659e-16,0,0,0,0,0,0,0,0,1,1.098431945,1.098431945,1.098431945,0,1.5054,1.5054,1.5054,0,"phi,normal",false,false,false,false,1,10000,0.5340576172,10000,0,1,1,true,true,paged,aos,ok,
cuda,global,all,100000,2.000171661,2.000076294,5.8145,4.5497,1.2648,0.8874,0.6182,0.9537280202,0.9437,0.9671,0.4476,0.2237,4.2785,0.9537280202,4.2785,42.785,23372677.34,0,0.005406495077,0.8738651282,true,0.005406495077,6.924660749e-05,0.0003859996001,2.247274656e-05,0,0,0,0,0,0,0,0,1,0.9537280202,0.9537280202,0.9537280202,0,4.2785,4.2785,4.2785,0,"phi,normal",false,false,false,false,1,100000,5.340576172,100000,0,1,1,true,true,paged,aos,ok,
cuda,global,all,1000000,2.000171661,2.000076294,6.1528,4.801,1.3516,8.4609,6.6563,9.83084774,9.8006,7.5456,4.8408,0.4241,39.1288,9.83084774,39.1288,39.1288,25556623.25,0,0.006410162008,0.8996876832,true,0.006410162008,7.064771896e-05,0.0003900821945,4.831315872e-05,0,0,0,0,0,0,0,0,1,9.83084774,9.83084774,9.83084774,0,39.1288,39.1288,39.1288,0,"phi,normal",false,false,false,false,1,1000000,53.40576172,1000000,0,1,1,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 10000 | 0 | 0.5007 | NA | 50.07 | 0 | 0 | ok
cpu | none | phi,normal | all | 100000 | 0 | 5.0576 | NA | 50.576 | 0 | 0 | ok
cpu | none | phi,normal | all | 1000000 | 0 | 50.7186 | NA | 50.7186 | 0 | 0 | ok
cuda | global | phi,normal | all | 10000 | 46.3212 | 1.5054 | 1.09843 | 150.54 | 0.00438517 | 0.873865 | ok
cuda | global | phi,normal | all | 100000 | 5.8145 | 4.2785 | 0.953728 | 42.785 | 0.0054065 | 0.873865 | ok
cuda | global | phi,normal | all | 1000000 | 6.1528 | 39.1288 | 9.83085 | 39.1288 | 0.00641016 | 0.899688 | ok
```

### Collision SVG Check: PASS

```bash
check-collision-svg '<local-path>'
```

```text
collision SVG generated and contains contact markers.
```

### Strict Report Validate strict_closed_cube_compressed_direct.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Validate strict_closed_cube_world_sparse_collision.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Validate strict_closed_cube_collision_world_benchmark.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Validate strict_closed_cube_sparse_query.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Validate strict_closed_cube_solver_contacts.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Validate strict_batch_query_benchmark.json: PASS

```bash
'<build>/tools/Release/adasdf_validate_report.exe' '<local-path>'
```

```text
Strict report valid: <local-path>
```

### Strict Report Summary CSV: PASS

```bash
'<build>/tools/Release/adasdf_collect_run_summary.exe' --inputs '<local-path>' --out '<local-path>'
```

```text
Run summary CSV: <local-path>
```

### Clean Check: PASS

```bash
'<local-path>' '<source>/scripts/check_repo_clean.py' '<source>'
```

```text
Repo clean check: PASS
```
