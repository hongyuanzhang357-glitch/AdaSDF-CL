# Install Validation Summary

- Source: `<source>`
- Build: `<build>`
- Install: `<install>`
- Config: `Release`

## Results

### Reuse Existing Build: PASS

```bash
'<local-path>' -c 'from pathlib import Path; build = Path(r'"'"''"'"''"'"'<build>'"'"''"'"''"'"'); cache = build / '"'"'CMakeCache.txt'"'"'; print('"'"'Reusing existing build:'"'"', build); raise SystemExit(0 if cache.is_file() else 1)'
```

```text
Reusing existing build: <local-path>
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
-- Up-to-date: <install>/bin/adasdf_recommend_build.exe
-- Up-to-date: <install>/bin/adasdf_build_demo_adaptive.exe
-- Up-to-date: <install>/bin/adasdf_collide_boxes_demo.exe
-- Up-to-date: <install>/bin/adasdf_query_mode_demo.exe
-- Up-to-date: <install>/bin/adasdf_expansion_quality.exe
-- Up-to-date: <install>/bin/adasdf_capabilities.exe
-- Up-to-date: <install>/bin/adasdf_mesh_check.exe
-- Up-to-date: <install>/bin/adasdf_mesh_clean.exe
-- Up-to-date: <install>/bin/adasdf_build_dense_sdf.exe
-- Up-to-date: <install>/bin/adasdf_build_adaptive_sdf.exe
-- Up-to-date: <install>/bin/adasdf_compress_adaptive_sdf.exe
-- Up-to-date: <install>/bin/adasdf_build_compressed_sdf.exe
-- Up-to-date: <install>/bin/adasdf_build_adaptive_sdf_preview.exe
...
```

### Package Configure: PASS

```bash
cmake -S '<source>/tests/package' -B '<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_pkg' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_pkg
```

### Package Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_pkg' --config Release --parallel -- /nodeReuse:false
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/tests/package/CMakeLists.txt
  test_find_package.vcxproj -> <build>_pkg\Release\test_find_package.exe
  Building Custom Rule <source>/tests/package/CMakeLists.txt
```

### Python CLI Wrapper Tests: PASS

```bash
'<local-path>' -m unittest discover -s '<source>/python/tests'
```

```text
.......................................
----------------------------------------------------------------------
Ran 39 tests in 0.302s

OK
```

### Installed Capabilities CLI: PASS

```bash
'<install>/bin/adasdf_capabilities.exe' --verbose
```

```text
AdaSDF-CL version: 1.13.0-alpha.1
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
- adaptive octree builder
- adaptive block partitioner
...
```

### Installed Mesh Check CLI: PASS

```bash
'<install>/bin/adasdf_mesh_check.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --readiness --out '<workspace>/build/install_validation_mesh_report.md'
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
Score: 100 / 100
Recommended for SDF build: yes
...
```

### Installed Mesh Clean CLI: PASS

```bash
'<install>/bin/adasdf_mesh_clean.exe' '<source>/tests/data/mesh_diagnostics/duplicate_and_degenerate_ascii.stl' '<workspace>/build/install_validation_cleaned.stl' --report '<workspace>/build/install_validation_cleanup_report.md'
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

### Installed Build Recommendation CLI: PASS

```bash
'<install>/bin/adasdf_recommend_build.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' --target-error 1e-3 --memory-mb 256 --use-case contact --out '<workspace>/build/install_validation_recommendation.md' --json '<workspace>/build/install_validation_recommendation.json' --emit-command
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

### Installed DenseSDF Build CLI: PASS

```bash
'<install>/bin/adasdf_build_dense_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_dense.sdfbin' --resolution 24 --padding 0.05 --report '<workspace>/build/install_validation_dense_report.md' --json '<workspace>/build/install_validation_dense_report.json'
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
Sampling time ms: 21.5094
BVH nodes: 0
BVH leaves: 0
Brute reference time ms: 0
Speedup vs brute reference: 0
Build time ms: 21.5911
Memory bytes: 111128
Reload validation: success
...
```

### Installed DenseSDF Info CLI: PASS

```bash
'<install>/bin/adasdf_info.exe' '<workspace>/build/install_validation_dense.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.13.0-alpha.1
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
Fine spacing: 0.0478261
Max level: 0
...
```

### Installed DenseSDF Query CLI: PASS

```bash
'<install>/bin/adasdf_query.exe' '<workspace>/build/install_validation_dense.sdfbin' --point 0.5 0.5 0.5
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

### Installed DenseSDF Collide CLI: PASS

```bash
'<install>/bin/adasdf_collide.exe' '<workspace>/build/install_validation_dense.sdfbin' '<workspace>/build/install_validation_dense.sdfbin' --max-contacts 4
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
Contact count: 1
Contact[0].point: 0.5 0.5 0.5
...
```

### Installed AdaptiveBlockSDF Build CLI: PASS

```bash
'<install>/bin/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_adaptive_block.sdfbin' --max-level 2 --block-resolution 5 --report '<workspace>/build/install_validation_adaptive_block_report.md' --json '<workspace>/build/install_validation_adaptive_block_report.json'
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
BVH nodes: 0
BVH leaves: 0
...
```

### Installed AdaptiveBlockSDF Info CLI: PASS

```bash
'<install>/bin/adasdf_info.exe' '<workspace>/build/install_validation_adaptive_block.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.13.0-alpha.1
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
Fine cell count: 4
Fine node count: 5
...
```

### Installed AdaptiveBlockSDF Query CLI: PASS

```bash
'<install>/bin/adasdf_query.exe' '<workspace>/build/install_validation_adaptive_block.sdfbin' --point 0.5 0.5 0.5
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

### Installed AdaptiveBlockSDF Collide CLI: PASS

```bash
'<install>/bin/adasdf_collide.exe' '<workspace>/build/install_validation_adaptive_block.sdfbin' '<workspace>/build/install_validation_adaptive_block.sdfbin' --max-contacts 4
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
Contact count: 1
Contact[0].point: 0.5 0.5 0.5
...
```

### Installed CompressedSDF Compress CLI: PASS

```bash
'<install>/bin/adasdf_compress_adaptive_sdf.exe' '<workspace>/build/install_validation_adaptive_block.sdfbin' '<workspace>/build/install_validation_compressed_block.sdfbin' --target-error 1e-3 --max-rank 5 --report '<workspace>/build/install_validation_compression_report.md' --json '<workspace>/build/install_validation_compression_report.json' --quality-report '<workspace>/build/install_validation_compression_quality.md'
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
Quality report: <local-path>
```

### Installed CompressedSDF Info CLI: PASS

```bash
'<install>/bin/adasdf_info.exe' '<workspace>/build/install_validation_compressed_block.sdfbin'
```

```text
AdaSDF-CL info
Library version: 1.13.0-alpha.1
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
Tucker/HOSVD compression: not implemented in v1.8.0-alpha
GPU-native compressed query: planned
...
```

### Installed CompressedSDF Query CLI: PASS

```bash
'<install>/bin/adasdf_query.exe' '<workspace>/build/install_validation_compressed_block.sdfbin' --point 0.5 0.5 0.5
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

### Installed CompressedSDF Collide CLI: PASS

```bash
'<install>/bin/adasdf_collide.exe' '<workspace>/build/install_validation_compressed_block.sdfbin' '<workspace>/build/install_validation_compressed_block.sdfbin' --max-contacts 4
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
Contact count: 1
Contact[0].point: 0.5 0.5 0.5
...
```

### Installed CompressedSDF Expansion Quality CLI: PASS

```bash
'<install>/bin/adasdf_expansion_quality.exe' '<workspace>/build/install_validation_compressed_block.sdfbin' --expansion global --global-resolution 16 --samples 200 --out '<workspace>/build/install_validation_compressed_quality.csv'
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
Fallback count: 0
Fallback rate: 0
...
```

### Installed CompressedSDF Benchmark Model CLI: PASS

```bash
'<install>/bin/adasdf_benchmark_batch_query.exe' --model '<workspace>/build/install_validation_compressed_block.sdfbin' --points 1000 --query-backend cpu --expansion none --out '<workspace>/build/install_validation_compressed_benchmark.csv'
```

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
cpu,none,all,1000,0,0,0,0,0,0.0155,0,NA,NA,0,0,0,1.9271,NA,1.9271,1927.1,518914.431,0,0,0,false,0,0,0,0,0,0,0,0,0,0,0,0,1,NA,NA,NA,NA,1.9271,1.9271,1.9271,0,"phi,normal",false,false,false,false,0,0,0,0,0,0,0,true,true,paged,aos,ok,
backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | max phi error | max normal error | status
cpu | none | phi,normal | all | 1000 | 0 | 1.9271 | NA | 1927.1 | 0 | 0 | ok
```

### Installed CompressedSDF One-Step Build CLI: PASS

```bash
'<install>/bin/adasdf_build_compressed_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_compressed_direct.sdfbin' --target-error 1e-3 --max-level 2 --block-resolution 5 --max-rank 5 --report '<workspace>/build/install_validation_compressed_direct_report.md' --compression-report '<workspace>/build/install_validation_compressed_direct_compression_report.md' --quality-report '<workspace>/build/install_validation_compressed_direct_quality.md'
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
Sampling time ms: 12.6746
BVH nodes: 0
BVH leaves: 0
Brute reference time ms: 0
Speedup vs brute reference: 0
Matrix-SVD blocks: 64
Dense fallback blocks: 0
Compression ratio: 0.911266
Max abs error: 4.996e-16
Quality samples: 4096
...
```

### Installed Sparse Query CLI: PASS

```bash
'<install>/bin/adasdf_sparse_query.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 0 --out '<workspace>/build/install_validation_sparse_results.csv' --report '<workspace>/build/install_validation_sparse_report.md'
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
Elapsed ms: 0.0071
Output mode: phi-only
Status: ok
```

### Installed Sparse Collide CLI: PASS

```bash
'<install>/bin/adasdf_sparse_collide.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 0 --early-exit --report '<workspace>/build/install_validation_sparse_collision.md'
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
Elapsed ms: 0.0027
Return code note: 10 means collision detected, not failure
Status: ok

Validation note: sparse_collide returned 10, which means collision detected and is expected for this fixture.
```

### Installed Contact Candidates CLI: PASS

```bash
'<install>/bin/adasdf_contact_candidates.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --top-k 4 --threshold 1e-3 --reduction-radius 0.02 --with-normal --out '<workspace>/build/install_validation_contact_candidates.csv' --report '<workspace>/build/install_validation_contact_candidates.md'
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

### Installed Contact Stabilization CLI: PASS

```bash
'<install>/bin/adasdf_stabilize_contacts.exe' '<workspace>/build/install_validation_contact_candidates.csv' --max-contacts 8 --patch-radius 0.02 --out '<workspace>/build/install_validation_stabilized_solver_contacts.csv' --json '<workspace>/build/install_validation_stabilized_solver_contacts.json' --report '<workspace>/build/install_validation_stabilized_solver_contacts.md'
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

### Installed Solver Contact Candidates CLI: PASS

```bash
'<install>/bin/adasdf_solver_contact_candidates.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1e-3 --top-k 32 --max-contacts 8 --patch-radius 0.02 --with-normal --out '<workspace>/build/install_validation_solver_contacts.csv' --candidates-out '<workspace>/build/install_validation_solver_raw_candidates.csv' --json '<workspace>/build/install_validation_solver_contacts.json' --report '<workspace>/build/install_validation_solver_contacts.md'
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

### Installed Contact Reduction Benchmark CLI: PASS

```bash
'<install>/bin/adasdf_benchmark_contact_reduction.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1e-3 --top-k 64 --max-contacts 8 --patch-radius 0.02 --repeat 2 --warmup 1 --csv '<workspace>/build/install_validation_contact_reduction_benchmark.csv' --json '<workspace>/build/install_validation_contact_reduction_benchmark.json' --report '<workspace>/build/install_validation_contact_reduction_benchmark.md'
```

```text
AdaSDF-CL contact reduction benchmark
sample_count: 6
raw_candidate_count: 2
patch_count: 2
solver_contact_count: 2
candidate_reduction_ratio: 1
avg_query_ms: 0.0158
avg_reduction_ms: 0.00555
avg_total_ms: 0.02135
max_contacts: 8
patch_radius: 0.02
repeat: 2
warmup: 1
Status: ok
```

### Installed Sparse Query Benchmark CLI: PASS

```bash
'<install>/bin/adasdf_benchmark_sparse_query.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 1 --mode phi-only --csv '<workspace>/build/install_validation_sparse_benchmark.csv'
```

```text
sample_count,repeat,warmup,total_ms,avg_ms,avg_us,avg_ns_per_sample,queried_samples_avg,early_exit_rate,mode,with_normal,threshold,top_k,status
6,2,1,0.0082,0.0041,4.1,683.333,6,0,phi-only,false,0,8,ok
Sparse benchmark mode: phi-only
Average ns per sample: 683.333
Status: ok
```

### Installed Active Block Selection CLI: PASS

```bash
'<install>/bin/adasdf_select_active_blocks.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --out '<workspace>/build/install_validation_active_blocks.csv' --report '<workspace>/build/install_validation_active_blocks.md' --json '<workspace>/build/install_validation_active_blocks.json'
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

### Installed Active Block Query CLI: PASS

```bash
'<install>/bin/adasdf_active_block_query.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --with-normal --out '<workspace>/build/install_validation_active_block_query.csv' --report '<workspace>/build/install_validation_active_block_query.md'
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
Query time ms: 0.1209
Status: ok

Validation note: active_block_query returned 10, which means collision detected and is expected for this fixture.
```

### Installed Active Block Cache Benchmark CLI: PASS

```bash
'<install>/bin/adasdf_benchmark_block_cache.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 1 --threshold 1.0 --selection-band 0.1 --compare-direct --csv '<workspace>/build/install_validation_block_cache_benchmark.csv' --report '<workspace>/build/install_validation_block_cache_benchmark.md'
```

```text
sample_count,repeat,warmup,active_block_count,expanded_block_count,cache_memory_bytes,cache_hit_rate,fallback_query_count,active_block_avg_ms,active_block_ns_per_sample,direct_avg_ms,direct_ns_per_sample,mode,threshold,selection_band,status
6,2,1,45,45,52560,1.000000,4,0.015050,2508.333333,0.003750,625.000000,phi-only,1.000000,0.100000,ok
Active block cache benchmark mode: phi-only
Average ns per sample: 2508.33
Status: ok
```

### Installed CUDA Active Block Query CLI: PASS

```bash
'<install>/bin/adasdf_cuda_active_block_query.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 --with-normal --out '<workspace>/build/install_validation_cuda_active_block_query.csv' --report '<workspace>/build/install_validation_cuda_active_block_query.md'
```

```text
AdaSDF-CL CUDA active block query
CUDA available: false
Sample count: 6
Queried count: 0
Result count: 0
Active blocks: 0
Expanded blocks: 0
GPU query count: 0
Fallback queries: 0
GPU memory bytes: 0
Selection time ms: 0
CPU expansion time ms: 0
Upload time ms: 0
Block upload time ms: 0
Sample upload time ms: 0
Kernel time ms: 0
Download time ms: 0
Total time ms: 0
Colliding: false
Min effective phi: 0
...
```

### Installed CUDA Active Block Cache Benchmark CLI: PASS

```bash
'<install>/bin/adasdf_benchmark_cuda_block_cache.exe' '<workspace>/build/install_validation_compressed_direct.sdfbin' '<source>/tests/data/samples/cube_sparse_samples.csv' --repeat 2 --warmup 0 --threshold 1.0 --selection-band 0.1 --compare-direct --csv '<workspace>/build/install_validation_cuda_block_cache_benchmark.csv' --report '<workspace>/build/install_validation_cuda_block_cache_benchmark.md'
```

```text
AdaSDF-CL CUDA active block cache benchmark
CUDA available: false
CUDA block cache benchmark mode: phi-only
Sample count: 6
Active blocks: 0
GPU memory bytes: 0
CUDA total avg ms: 0
CUDA kernel avg ms: 0
CUDA upload avg ms: 0
CUDA download avg ms: 0
Average ns per sample: 0
CPU active avg ms: 0
Direct sparse avg ms: 0
Error: CUDA active block benchmark skipped because CUDA is unavailable.
Status: skipped

Validation note: benchmark_cuda_block_cache returned 20, which means CUDA was unavailable and the run was skipped as expected.
```

### Installed AdaptiveBlockSDF Dry Run CLI: PASS

```bash
'<install>/bin/adasdf_build_adaptive_sdf.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_adaptive_dryrun.sdfbin' --max-level 2 --block-resolution 5 --dry-run --report '<workspace>/build/install_validation_adaptive_dryrun_plan.md'
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

### Installed Adaptive Builder Preview CLI: PASS

```bash
'<install>/bin/adasdf_build_adaptive_sdf_preview.exe' '<source>/tests/data/mesh_diagnostics/closed_cube_ascii.stl' '<workspace>/build/install_validation_adaptive_preview.sdfbin' --target-error 1e-3 --memory-mb 512 --dry-run --plan '<workspace>/build/install_validation_adaptive_preview_plan.md'
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

### Package Run: PASS

```bash
'<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_pkg/Release/test_find_package.exe'
```

```text
AdaSDF-CL package consumer
Version: 1.13.0-alpha.1
Point: 1 2 3
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Batch query points: 2
CUDA batch backend available: false
CPU backend available: true
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_ds' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_ds' --config Release --parallel -- /nodeReuse:false
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
  adasdf_downstream.vcxproj -> <build>_ds\Release\adasdf_downstream.exe
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
```

### Downstream Run: PASS

```bash
'<workspace>/build/adasdf_cl-v1_13_ci_hotfix_alpha_validation_reuse_ds/Release/adasdf_downstream.exe'
```

```text
AdaSDF-CL downstream example
Version: 1.13.0-alpha.1
CPU backend: available
No .sdfbin supplied; running core-free demo adaptive path.
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Demo batch query points: 3
CUDA batch backend: unavailable
Demo colliding: true
Demo contacts: 4
```
