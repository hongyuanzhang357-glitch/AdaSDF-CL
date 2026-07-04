# AdaSDF-CL

Adaptive Signed Distance Field Collision Library

Status: 1.15.0-alpha / research preview
Build system: CMake
License: MIT
Tests: CTest

AdaSDF-CL is an alpha collision and contact library built around signed distance fields. It provides an FCL-style API for distance, collision, and contact queries while keeping CUDA, FCL, native Python bindings, and full adaptive backend work optional or future-facing.

AdaSDF-CL is an FCL-style SDF collision backend under development. It complements FCL by providing signed-distance queries, penetration depth, contact normals, batch query, expanded-SDF quality audit, CUDA query paths, and a first SDF-native multi-object CollisionWorld broadphase. It is not a drop-in FCL replacement.

v1.15.0-alpha adds strict JSON/CSV reporting and reproducibility manifests for
automation, benchmark provenance, install validation, alpha validation, and
external regression tracking. It introduces dependency-free report writers,
validators, collectors, manifest CLIs, Python wrapper support, and
`--strict-json` / `--case-id` on selected core CLIs. It does not change the core
collision algorithms or introduce nlohmann_json, Python, FCL, or existing-core
dependencies into the report module.

v1.14.0-alpha adds `CollisionWorld`, deterministic AABB broadphase,
sample-based world sparse collision, solver-ready world contact candidate
export, simple CSV scene loading, world benchmark tooling, and Python wrapper
helpers for the new world CLIs. It does not implement exact mesh-vs-mesh
contact, FCL fallback, a contact solver, CCD, ROS/MoveIt, pybind11, or a
general scene graph.

v1.13.0-alpha adds solver-aware contact candidate stabilization. It clusters
and reduces SDF contact candidates under a user-defined contact budget,
enforces deterministic ordering and normal consistency, and exports
solver-ready contact candidates for hard-contact simulation workflows. It does
not implement a contact solver, impulses, friction, or solver constraints.

v1.13.0-alpha.1 is a CI hotfix for the v1.13 contact stabilization release. It
keeps the solver-aware contact APIs unchanged and updates GitHub Actions to
reuse the already-built CI tree during install validation.

v1.13.0-alpha.2 is a CI hotfix for the v1.13 tag workflow. It keeps the
solver-aware contact APIs unchanged and limits GitHub Actions build parallelism
to reduce Ubuntu runner peak load when `main` and tag workflows are triggered
for the same commit.

v1.12.0-alpha adds optional CPU TriangleBVH acceleration for DenseSDF,
AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF builder sampling. The default
builder path remains `--accel brute`; `--accel bvh` selects the new
deterministic BVH path, `--threads N` enables stdlib parallel sampling, and
`adasdf_benchmark_builder_acceleration` reports builder speedups.

v1.11.0-alpha added a CUDA active block cache baseline. It selects and expands
active blocks on CPU, uploads expanded local dense blocks to GPU, and runs
CUDA local block interpolation for phi-only or phi+normal sparse queries. It
is not GPU-native compressed SVD reconstruction.

v1.10.0-alpha added contact-aware active block expansion and a CPU expanded
block cache for compressed/adaptive SDF runtime queries. It selects local
blocks from sparse samples, expands only those blocks, reports cache/fallback
sources, and avoids global expansion when a contact workflow only needs a
small local working set.

v1.9.0-alpha added sparse point-to-SDF collision queries and contact candidate
extraction. Sparse query defaults to phi-only, collision-only mode supports
early exit, sample radius uses `effective_phi = phi - radius`, and Top-K
candidate reduction keeps hard-contact constraint budgets small. These
candidates are not full solver contacts or a complete contact manifold.

v1.8.1-alpha adds a lightweight pure-Python CLI wrapper under
`python/adasdf_cli`. The wrapper calls installed AdaSDF-CL command-line tools
through `subprocess`, returns dataclass results, supports dry-run command
preview, and uses only the Python standard library. It is not a native
pybind11 binding or C++ extension module.

v1.8.0-alpha added surrogate-guided build recommendation for public core-free
STL workflows. `adasdf_recommend_build` can recommend DenseSDF,
AdaptiveBlockSDF, or CompressedAdaptiveBlockSDF parameters from an STL, target
near-surface error, memory budget, and use case. The recommender is
deterministic and experimental; it is not a universal trained model, not fully
trained, and not an optimality guarantee.

v1.7.0-alpha introduced matrix-SVD low-rank compression for adaptive block
SDFs. Each adaptive dense block can be stored as low-rank factors with
per-block error auditing and dense fallback when compression cannot meet the
target. CUDA, FCL, Python, and the existing research core remain optional;
CPU-only builds remain fully usable.

The original `v1.0.2-alpha`, `v1.0.2-alpha.1`, `v1.0.3-alpha`, `v1.1.0-alpha`,
`v1.1.1-alpha`, `v1.2.0-alpha`, `v1.3.0-alpha`, `v1.4.0-alpha`,
`v1.5.0-alpha`, `v1.6.0-alpha`, `v1.7.0-alpha`, `v1.8.0-alpha`,
`v1.8.1-alpha`, `v1.9.0-alpha`, `v1.10.0-alpha`, `v1.11.0-alpha`,
`v1.12.0-alpha`, `v1.13.0-alpha`, `v1.13.0-alpha.1`, and
`v1.13.0-alpha.2`, and `v1.14.0-alpha` tags are retained for traceability. The
recommended public pre-release is `v1.15.0-alpha`.

## What Is AdaSDF-CL?

AdaSDF-CL is a research-preview adaptive SDF collision library. It is best used
today to explore SDF-native distance, contact, batch query, CUDA expanded query,
and expanded-grid accuracy auditing. It is not an industrial-grade certified
collision engine and does not yet replace FCL.

## Current Status

| Capability | Status |
| --- | --- |
| Core-free demo adaptive SDF | Implemented |
| FCL-style collision API | Implemented |
| Contact point / normal / penetration depth | Implemented |
| CPU direct/global/block query | Implemented |
| CUDA expanded query | Implemented / experimental |
| Expansion quality audit | Implemented |
| Sign mismatch / near-surface mismatch metrics | Implemented |
| ASCII / binary STL reader for diagnostics | Implemented |
| STL mesh diagnostics preflight | Implemented |
| SDF build readiness scoring | Implemented |
| Mesh repair suggestions | Implemented |
| Safe mesh cleanup | Implemented |
| ASCII STL writer | Implemented |
| Standalone uniform DenseSDF builder | Implemented |
| DenseSDF `.sdfbin` read/write | Implemented |
| Adaptive octree/block SDF builder | Implemented |
| Adaptive block `.sdfbin` read/write | Implemented |
| Matrix-SVD block compression | Implemented |
| Compressed adaptive block SDF model | Implemented |
| Compressed block `.sdfbin` read/write | Implemented |
| Surrogate-guided build recommendation | Implemented / experimental |
| `adasdf_recommend_build` | Implemented |
| Python CLI wrapper | Implemented |
| Sparse point-to-SDF collision | Implemented |
| Collision-only sparse early exit | Implemented |
| Sample-radius collision proxy | Implemented |
| Top-K contact candidate reduction | Implemented |
| Solver-aware contact candidates | Implemented |
| Contact patch clustering and budget | Implemented |
| Solver contact CSV/Markdown/JSON-like export | Implemented |
| CollisionWorld broadphase | Implemented |
| Multi-object sample-based SDF collision | Implemented |
| World solver-ready contact export | Implemented |
| Strict JSON reproducibility reports | Implemented |
| Strict report validation and run summary CSV | Implemented |
| Sparse query benchmark | Implemented |
| Contact-aware active block cache | Implemented |
| Active block cache benchmark | Implemented |
| CUDA active block cache | Implemented / experimental |
| CUDA local active-block query | Implemented / experimental |
| Native Python / pybind11 binding | Planned |
| Adaptive compressed builder interface preview | Planning tool / implemented matrix-SVD status |
| Existing-core sampled expansion bridge | Existing-core only / partial |
| Trained surrogate model integration | Planned |
| Tucker/HOSVD compression | Planned |
| GPU-native compressed query | Planned |
| Complex mesh repair / hole filling | Planned |
| FCL fallback backend | Planned |
| CCD | Planned |

Detailed capability references:

- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/fcl_complement_strategy.md`
- `docs/query_backend_matrix.md`
- `docs/contact_output_matrix.md`
- `docs/mesh_diagnostics.md`
- `docs/mesh_readiness.md`
- `docs/mesh_cleanup.md`
- `docs/dense_sdf_builder.md`
- `docs/stl_to_sdf_public_workflow.md`
- `docs/adaptive_octree_block_builder.md`
- `docs/adaptive_block_sdfbin_format.md`
- `docs/low_rank_block_compression.md`
- `docs/compressed_block_sdfbin_format.md`
- `docs/stl_to_compressed_sdf_workflow.md`
- `docs/compression_quality_metrics.md`
- `docs/surrogate_guided_recommendation.md`
- `docs/recommended_build_workflow.md`
- `docs/recommendation_profiles.md`
- `docs/python_cli_wrapper.md`
- `docs/sparse_sdf_collision.md`
- `docs/contact_candidate_api.md`
- `docs/solver_aware_contact_candidates.md`
- `docs/contact_budget.md`
- `docs/contact_clustering.md`
- `docs/solver_contact_export.md`
- `docs/collision_world_scene_format.md`
- `docs/collision_world_v1_14_report.md`
- `docs/strict_report_schema.md`
- `docs/reproducibility_manifest.md`
- `docs/run_summary_csv.md`
- `docs/hard_contact_collision_budget.md`
- `docs/contact_reduction_benchmarking.md`
- `docs/sparse_query_benchmarking.md`
- `docs/active_block_expansion_cache.md`
- `docs/contact_aware_block_selection.md`
- `docs/block_cache_benchmarking.md`
- `docs/cuda_active_block_cache.md`
- `docs/cuda_active_block_benchmarking.md`
- `docs/runtime_memory_strategy.md`

## Sparse SDF Collision And Contact Candidates

v1.9.0-alpha adds sparse point collision as a first-class SDF workflow:

```bash
adasdf_sparse_query model.sdfbin samples.csv --threshold 0 --out sparse_results.csv

adasdf_sparse_collide model.sdfbin samples.csv --threshold 0 --early-exit

adasdf_contact_candidates model.sdfbin samples.csv \
  --top-k 8 \
  --threshold 1e-3 \
  --reduction-radius 0.02 \
  --with-normal \
  --out candidates.csv

adasdf_benchmark_sparse_query model.sdfbin samples.csv \
  --repeat 100 \
  --mode phi-only
```

`adasdf_sparse_collide` returns code `10` when collision is detected. That is a
successful collision status, not a program failure.

Direct compressed query can be useful for sparse queries, debugging, fallback,
and small point sets. It is not the main high-throughput GPU path. v1.10 adds
the CPU active block cache so contact workflows can expand only selected local
blocks instead of globally expanding the entire model.

## Solver-aware contact candidates

v1.13.0-alpha stabilizes sparse SDF contact candidates into a small
solver-ready contact set:

```bash
adasdf_solver_contact_candidates model.sdfbin samples.csv \
  --threshold 1e-3 \
  --top-k 32 \
  --max-contacts 8 \
  --patch-radius 0.02 \
  --with-normal \
  --out solver_contacts.csv \
  --report solver_contacts.md

adasdf_benchmark_contact_reduction model.sdfbin samples.csv \
  --threshold 1e-3 \
  --top-k 64 \
  --max-contacts 8 \
  --repeat 100 \
  --csv contact_reduction_benchmark.csv
```

This exports solver-ready candidates, not solver constraints. It does not
compute impulses or friction forces. Hard-contact solvers should keep contact
budgets small and use `--max-contacts`, `--patch-radius`, and normal
consistency to control solver load.

## CollisionWorld Broadphase

v1.14.0-alpha adds a simple multi-object SDF scene path:

```bash
adasdf_world_broadphase scene.csv \
  --out world_pairs.csv \
  --report world_broadphase.md

adasdf_world_sparse_collide scene.csv \
  --threshold 0 \
  --early-exit \
  --out world_hits.csv \
  --report world_sparse.md

adasdf_world_solver_contacts scene.csv \
  --threshold 1e-3 \
  --top-k 32 \
  --max-contacts 8 \
  --out world_solver_contacts.csv \
  --report world_solver_contacts.md

adasdf_benchmark_collision_world scene.csv \
  --mode sparse \
  --repeat 10 \
  --csv collision_world_benchmark.csv
```

The scene file is CSV-based; see `docs/collision_world_scene_format.md`.
Broadphase is deterministic AABB filtering. Narrowphase is sample-based SDF
collision over object sample sets, not exact mesh-vs-mesh contact.
`adasdf_world_sparse_collide` returns code `10` when collision is detected;
that is a successful collision status.

## Active Block Expansion Cache

```bash
adasdf_select_active_blocks model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --out active_blocks.csv

adasdf_active_block_query model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --with-normal \
  --out active_query.csv

adasdf_benchmark_block_cache model.sdfbin samples.csv \
  --repeat 10 \
  --compare-direct \
  --csv block_cache_benchmark.csv
```

`adasdf_active_block_query` returns code `10` when collision is detected. That
is a successful collision status, not a program failure.

## CUDA Active Block Cache

```bash
adasdf_cuda_active_block_query model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --phi-only \
  --out cuda_active_query.csv

adasdf_benchmark_cuda_block_cache model.sdfbin samples.csv \
  --repeat 20 \
  --warmup 5 \
  --mode phi-only \
  --compare-direct \
  --csv cuda_block_cache_benchmark.csv
```

`adasdf_cuda_active_block_query` returns code `10` when collision is detected
and code `20` when CUDA is unavailable. Return code `20` is an explicit skip,
not a repository or model failure.

## Python CLI Wrapper

v1.8.1-alpha adds a source-tree Python package:

```python
import adasdf_cli as adasdf

adasdf.mesh_check("model.stl", readiness=True, out="mesh_report.md")

rec = adasdf.recommend_build(
    "model.stl",
    target_error=1e-3,
    memory_mb=512,
    use_case="contact",
    out="recommendation.md",
)
print(rec.recommended_command)

adasdf.build_compressed_sdf(
    "model.stl",
    "model_compressed.sdfbin",
    target_error=1e-3,
    max_level=4,
    block_resolution=8,
    max_rank=8,
)

q = adasdf.query("model_compressed.sdfbin", point=[0, 0, 0])
print(q.phi)

hit = adasdf.sparse_collide(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=0.0,
    early_exit=True,
)
print(hit.colliding)

cand = adasdf.contact_candidates(
    "model_compressed.sdfbin",
    "samples.csv",
    top_k=8,
    threshold=1e-3,
    reduction_radius=0.02,
)
print(cand.candidate_count)
```

After building or installing AdaSDF-CL tools:

```bash
set ADASDF_BIN=C:\path\to\install\bin
set PYTHONPATH=C:\path\to\AdaSDF-CL\python
```

On Linux/macOS:

```bash
export ADASDF_BIN=/path/to/install/bin
export PYTHONPATH=/path/to/AdaSDF-CL/python
```

Every helper also accepts `bin_dir=` and `dry_run=True`. The wrapper is a
subprocess-based convenience layer. It is not pybind11, not a native Python
binding, and not a C++ extension module. Native Python bindings remain planned.

## Surrogate-Guided Build Recommendation

v1.8.0-alpha adds:

```bash
adasdf_recommend_build model.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --json recommendation.json --emit-command
```

The tool recommends one of:

- `DenseSDF`;
- `AdaptiveBlockSDF`;
- `CompressedAdaptiveBlockSDF`.

The recommended public STL workflow is:

- v1.5: build uniform DenseSDF assets.
- v1.6: build adaptive octree/block dense assets.
- v1.7: build matrix-SVD compressed adaptive block assets.
- v1.8: recommend route and parameters before choosing a build command.

The v1.8 recommender is deterministic and heuristic-calibrated. It is not a
universal trained model, not fully trained, and not an optimality guarantee.
- `docs/adaptive_dense_vs_compressed_sdf.md`
- `docs/stl_to_adaptive_sdf_workflow.md`
- `docs/adaptive_vs_dense_sdf.md`
- `docs/adaptive_builder_preview.md`
- `docs/stl_import_audit.md`
- `docs/public_positioning.md`

## Quick Start

```bash
git clone https://github.com/hongyuanzhang357-glitch/AdaSDF-CL.git
cd AdaSDF-CL
git checkout v1.15.0-alpha

cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_BENCHMARKS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install

install/bin/adasdf_capabilities --verbose
install/bin/adasdf_mesh_check tests/data/mesh_diagnostics/closed_cube_ascii.stl --readiness --out mesh_report.md
install/bin/adasdf_mesh_clean tests/data/mesh_diagnostics/duplicate_and_degenerate_ascii.stl cleaned.stl --report cleanup_report.md
install/bin/adasdf_recommend_build tests/data/mesh_diagnostics/closed_cube_ascii.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --json recommendation.json --emit-command
install/bin/adasdf_build_dense_sdf tests/data/mesh_diagnostics/closed_cube_ascii.stl cube_dense.sdfbin --resolution 32 --padding 0.05 --report dense_report.md
install/bin/adasdf_info cube_dense.sdfbin
install/bin/adasdf_query cube_dense.sdfbin --point 0.5 0.5 0.5
install/bin/adasdf_collide cube_dense.sdfbin cube_dense.sdfbin --max-contacts 4
install/bin/adasdf_build_adaptive_sdf tests/data/mesh_diagnostics/closed_cube_ascii.stl cube_adaptive_block.sdfbin --target-error 1e-3 --max-level 4 --block-resolution 8 --report adaptive_report.md
install/bin/adasdf_info cube_adaptive_block.sdfbin
install/bin/adasdf_query cube_adaptive_block.sdfbin --point 0.5 0.5 0.5
install/bin/adasdf_collide cube_adaptive_block.sdfbin cube_adaptive_block.sdfbin --max-contacts 4
install/bin/adasdf_compress_adaptive_sdf cube_adaptive_block.sdfbin cube_compressed.sdfbin --target-error 1e-3 --max-rank 8 --report compression_report.md --quality-report compression_quality.md
install/bin/adasdf_build_compressed_sdf tests/data/mesh_diagnostics/closed_cube_ascii.stl cube_compressed_direct.sdfbin --target-error 1e-3 --max-level 4 --block-resolution 8 --max-rank 8 --report compressed_build_report.md --compression-report compression_report.md --quality-report compression_quality.md
install/bin/adasdf_info cube_compressed.sdfbin
install/bin/adasdf_query cube_compressed.sdfbin --point 0.5 0.5 0.5
install/bin/adasdf_collide cube_compressed.sdfbin cube_compressed.sdfbin --max-contacts 4
install/bin/adasdf_sparse_query cube_compressed.sdfbin tests/data/samples/cube_sparse_samples.csv --threshold 0 --out sparse_results.csv
install/bin/adasdf_sparse_collide cube_compressed.sdfbin tests/data/samples/cube_sparse_samples.csv --threshold 0 --early-exit
install/bin/adasdf_contact_candidates cube_compressed.sdfbin tests/data/samples/cube_sparse_samples.csv --top-k 4 --threshold 1e-3 --reduction-radius 0.02 --with-normal --out contact_candidates.csv
install/bin/adasdf_world_broadphase scene.csv --out world_pairs.csv --report world_broadphase.md
install/bin/adasdf_world_sparse_collide scene.csv --threshold 0 --early-exit --out world_hits.csv --report world_sparse.md
install/bin/adasdf_world_solver_contacts scene.csv --threshold 1e-3 --top-k 32 --max-contacts 8 --out world_solver_contacts.csv --report world_solver_contacts.md
install/bin/adasdf_benchmark_collision_world scene.csv --mode sparse --repeat 10 --csv collision_world_benchmark.csv
install/bin/adasdf_benchmark_sparse_query cube_compressed.sdfbin tests/data/samples/cube_sparse_samples.csv --repeat 10 --warmup 1 --mode phi-only --csv sparse_benchmark.csv
install/bin/adasdf_build_adaptive_sdf_preview tests/data/mesh_diagnostics/closed_cube_ascii.stl cube_adaptive_preview.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
install/bin/adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
install/bin/adasdf_build_demo_adaptive cube_adaptive.sdfbin --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
install/bin/adasdf_info cube_adaptive.sdfbin
install/bin/adasdf_query cube_adaptive.sdfbin --point 0 0 0
install/bin/adasdf_expansion_quality cube_adaptive.sdfbin --expansion global --global-resolution 64 --samples 10000
install/bin/adasdf_expansion_quality cube_adaptive.sdfbin --expansion block --blocks all --block-resolution 32 --samples 10000
install/bin/adasdf_collide_boxes_demo --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view collision.svg
install/bin/adasdf_query_mode_demo --backend cpu --expansion none --points 100000
install/bin/adasdf_query_mode_demo --backend cuda --expansion global --points 100000
install/bin/adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --kernel-only --reuse-resident --warmup 10 --repeat 50 --out benchmark.csv

set ADASDF_BIN=%CD%\install\bin
set PYTHONPATH=%CD%\python
python -m unittest discover -s python/tests
```

## Clone-Only Demo Workflow

The public repository supports a no-extra-core workflow:

```text
demo surrogate recommendation
-> demo adaptive SDF metadata
-> demo adaptive .sdfbin
-> point query
-> two-box collision/contact
-> SVG collision view
-> expansion quality audit
```

On Windows, installed tools usually have `.exe` suffixes, for example `install/bin/adasdf_recommend_demo.exe`.

Generated `.sdfbin` and `.svg` files should stay in build, install, or temporary directories and should not be committed to the source tree.

## Public STL-To-SDF Workflow

v1.5.0-alpha introduced a standalone uniform dense SDF builder. v1.6.0-alpha
adds a public adaptive octree/block SDF builder. v1.7.0-alpha adds
matrix-SVD low-rank compression for adaptive blocks with error auditing and
dense fallback.

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_build_dense_sdf model_clean.stl model_dense.sdfbin --resolution 64 --padding 0.05
adasdf_info model_dense.sdfbin
adasdf_query model_dense.sdfbin --point 0 0 0
adasdf_collide model_dense.sdfbin model_dense.sdfbin --max-contacts 4
```

## Public STL-to-adaptive-block-SDF workflow

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_build_adaptive_sdf model_clean.stl model_adaptive.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --report adaptive_report.md
adasdf_info model_adaptive.sdfbin
adasdf_query model_adaptive.sdfbin --point 0 0 0
adasdf_collide model_adaptive.sdfbin model_adaptive.sdfbin --max-contacts 4
```

DenseSDF builder:

- Uniform grid.
- Simple baseline.
- Implemented since v1.5.

AdaptiveBlockSDF builder:

- Octree-refined domain.
- Block-wise dense SDF.
- Implemented since v1.6.

CompressedAdaptiveBlockSDF builder:

- Matrix-SVD compressed adaptive blocks.
- Implemented since v1.7.
- May contain dense fallback blocks when compression cannot satisfy error targets.

Surrogate-guided build recommendation is implemented in v1.8 through
`adasdf_recommend_build`.

## Public STL-to-compressed-SDF workflow

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_build_compressed_sdf model_clean.stl model_compressed.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --max-rank 8 --report build_report.md --compression-report compression_report.md --quality-report quality_report.md
adasdf_info model_compressed.sdfbin
adasdf_query model_compressed.sdfbin --point 0 0 0
adasdf_collide model_compressed.sdfbin model_compressed.sdfbin --max-contacts 4
```

Compressed models can be queried directly on CPU and can be expanded for
CPU/CUDA benchmark workflows. Tucker/HOSVD compression, surrogate-guided
parameter recommendation, and GPU-native compressed query remain planned work.

Open meshes can be built as unsigned distance fields:

```bash
adasdf_build_dense_sdf open_mesh.stl open_mesh_dense.sdfbin --resolution 64 --unsigned
```

## Adaptive Builder Interface Preview

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

The adaptive builder preview remains a planning tool. Real block-wise dense
adaptive construction is available through `adasdf_build_adaptive_sdf` in
v1.6.0-alpha. Matrix-SVD low-rank block compression is available in
v1.7.0-alpha through `adasdf_build_compressed_sdf` and
`adasdf_compress_adaptive_sdf`.

## What Works Now

- `adasdf_capabilities` for a quick implemented/partial/planned feature summary.
- `adasdf_mesh_check` for ASCII/binary STL mesh diagnostics and SDF build readiness before SDF construction.
- `adasdf_mesh_clean` for safe cleanup and clean ASCII STL export.
- `TriangleMesh`, `STLReader`, `STLWriter`, `MeshDiagnostics`, `MeshReadiness`, `MeshCleanup`, and `MeshDiagnosticsWriter`.
- Uniform `DenseSDFModel`, `DenseSDFBuilder`, `ADASDF_DENSE_SDFBIN_V1`, and
  `adasdf_build_dense_sdf` for a public core-free STL-to-uniform-SDF path.
- `AdaptiveOctree`, `AdaptiveBlockPartitioner`, `AdaptiveBlockSDFBuilder`,
  `AdaptiveBlockSDFModel`, `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`, and
  `adasdf_build_adaptive_sdf` for a public core-free STL-to-adaptive-block-SDF
  path.
- `SmallMatrixSVD`, `CompressedSDFBlock`, `BlockLowRankCompressor`,
  `CompressedAdaptiveBlockSDFModel`, `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`,
  `adasdf_compress_adaptive_sdf`, and `adasdf_build_compressed_sdf` for a
  public core-free low-rank adaptive block compression path.
- `adasdf_build_adaptive_sdf_preview` for a dry-run preview of adaptive build
  plans and future low-rank stages.
- Core-free analytic box SDF model.
- Demo `.sdfbin` format `ADASDF_DEMO_SDFBIN_V1`.
- Demo adaptive `.sdfbin` format `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- Demo surrogate recommender that maps target error and memory constraints to adaptive demo parameters.
- Demo adaptive builder that creates octree metadata, block metadata, and rank metadata for an analytic box backend.
- `adasdf_recommend_demo`, `adasdf_build_demo_adaptive`, `adasdf_collide_boxes_demo`, `adasdf_make_demo_box`, `adasdf_info`, `adasdf_query`, and `adasdf_collide`.
- SVG collision visualization with box outlines, contact points, normal arrows, backend/method labels, minimum distance, and contact counts.
- Installable CMake package consumed with `find_package(AdaSDFCL CONFIG REQUIRED)`.
- CPU batch query API for signed distance, gradient, and normal queries.
- Query-mode configuration for CPU direct, CPU global-expanded, CPU block-expanded, CUDA global-expanded, and CUDA block-expanded queries.
- `ExpandedSDF`, `SDFExpander`, and `QueryEngine` for reusable pre-expanded query preparation.
- Optional CUDA resident expanded SDF backend for analytic/demo adaptive box SDFs.
- CUDA query workspace reuse for points, phi, and normal buffers when `--reuse-resident` is enabled.
- CUDA phi-only expanded SDF kernel for signed-distance-only benchmark comparison.
- Explicit benchmark output modes: `--output phi` and `--output phi,normal`.
- CUDA device-only benchmark mode for no-download GPU-side timing.
- ExpandedSDF quality audit comparing expanded queries against direct model queries.
- Sign mismatch, ambiguous sign, near-surface sign mismatch, and fallback-rate metrics.
- `adasdf_expansion_quality` CLI for global/block expansion quality reports.
- `examples/11_capability_walkthrough.cpp` for a CPU-only public capability tour.
- `examples/12_mesh_diagnostics_demo.cpp` for a CPU-only STL diagnostics walkthrough.
- `examples/13_mesh_cleanup_demo.cpp` for a CPU-only safe cleanup walkthrough.
- Deterministic benchmark point generation and `adasdf_benchmark_batch_query` with backend, expansion, block selection, memory, setup, timing breakdown, warmup/repeat statistics, kernel-only mode, workspace reuse fields, block lookup fields, and error columns.

## Backend Boundary

AdaSDF-CL currently has two practical modes.

### Core-Free Public Demo

The public repository can be configured, built, tested, installed, and consumed by external CMake projects without the original research core.

v0.9 supports a public workflow:

```text
target error + memory constraints
-> demo surrogate recommendation
-> demo adaptive SDF metadata
-> demo adaptive .sdfbin
-> point query
-> two-box collision
-> SVG collision view
```

The demo adaptive builder uses analytic box SDF queries and demo adaptive
metadata to exercise the public workflow. The v1.5 DenseSDF builder is the
implemented public STL-to-uniform-SDF path. The v1.6 AdaptiveBlockSDF builder
is the implemented public STL-to-adaptive-block dense path. The v1.7
CompressedAdaptiveBlockSDF path is the first public low-rank adaptive block
compression workflow.

### Existing-Core Enhanced Build

When configured with `ADASDF_CL_USE_EXISTING_CORE=ON` and a valid `ADASDF_CL_EXISTING_ROOT`, AdaSDF-CL can use the existing research core to build and read existing-core adaptive `.sdfbin` assets. That path remains separate from the v0.9 demo backend.

## Build From Source

CMake options:

- `ADASDF_CL_BUILD_EXAMPLES`: build example executables.
- `ADASDF_CL_BUILD_TESTS`: build and register CTest tests.
- `ADASDF_CL_BUILD_BENCHMARKS`: build benchmark tools, default `ON`.
- `ADASDF_CL_ENABLE_DEMO_BACKEND`: enable the core-free demo backend, default `ON`.
- `ADASDF_CL_ENABLE_DEMO_SURROGATE`: enable the demo surrogate recommender, default `ON`.
- `ADASDF_CL_ENABLE_COLLISION_VIEWER`: enable SVG collision visualization, default `ON`.
- `ADASDF_CL_USE_EXISTING_CORE`: use the existing OctreeBlockLowRankSDF core when available.
- `ADASDF_CL_ENABLE_ADAPTIVE_BUILDER`: enable the public adaptive builder API.
- `ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER`: enable the placeholder DASH-SDF recommender API.
- `ADASDF_CL_ENABLE_CUDA`: enable the optional CUDA batch query backend, default `OFF`.

CMake configure summary reports:

```text
CUDA backend: ON/OFF
CUDA toolkit: found/not found
Benchmarks: ON/OFF
```

## STL Mesh Diagnostics

Before building an SDF from STL, users can inspect the mesh:

```bash
adasdf_mesh_check model.stl --out mesh_report.md
adasdf_mesh_check model.stl --json mesh_report.json --tolerance 1e-12
adasdf_mesh_check model.stl --readiness --out mesh_readiness_report.md
```

The diagnostic report includes triangle count, AABB, watertight status,
boundary edges, non-manifold edges, degenerate triangles, duplicate triangles,
connected components, isolated vertices, and scale warnings.

`v1.2.0-alpha` introduced STL mesh diagnostics as a preflight step. v1.4 adds
safe cleanup for obvious duplicate/degenerate elements. v1.5 adds a standalone
uniform DenseSDF builder. v1.6 adds a standalone adaptive octree/block dense
SDF builder. v1.7 adds matrix-SVD compressed adaptive block SDF output. These
builders still do not repair self-intersections.

## SDF Build Readiness

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
```

The readiness report converts raw mesh diagnostics into severity-ranked issues,
a 0-100 readiness score, and recommended preprocessing steps before adaptive SDF
construction.

Readiness is a preflight heuristic, not an industrial certification or
automatic mesh repair. The input STL is never modified.

See `docs/mesh_diagnostics.md`, `docs/mesh_readiness.md`, and
`docs/stl_import_audit.md`.

## Safe Mesh Cleanup

```bash
adasdf_mesh_clean bad.stl cleaned.stl --report cleanup_report.md
adasdf_mesh_check bad.stl --readiness --clean-out cleaned.stl --clean-report cleanup_report.md
adasdf_mesh_check cleaned.stl --readiness --out cleaned_report.md
```

The cleanup pass can merge near-duplicate vertices, remove degenerate
triangles, remove duplicate triangles, remove unused vertices, remap triangle
indices, and write a clean ASCII STL. Cleanup never overwrites the input STL.

Cleanup does not fill holes, repair self-intersections, boolean reconstruct a
surface, infer units, or change model scale. It is an SDF build preflight
utility, not an industrial CAD repair engine. Operations that may change
topology are reported in cleanup stats and before/after Markdown reports.

See `docs/mesh_cleanup.md`.

## CPU/GPU Batch-Query Benchmark

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion none --out cpu_direct.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion global --out cuda_global.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion block --blocks 0,1,2 --out cuda_blocks.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_kernel.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi,normal --reuse-resident --warmup 10 --repeat 50 --out cuda_global_full.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --device-only --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_device_only.csv
```

The CUDA backend is optional. If CUDA is not available, AdaSDF-CL remains fully usable in CPU mode and CUDA benchmark rows are marked as skipped.

```text
CUDA backend unavailable
```

AdaSDF-CL supports CUDA batch queries over pre-expanded global or block dense
SDF data for queryable public models, including compressed adaptive SDFs after
sampled expansion. GPU-native compressed SDF query is planned but not yet
implemented.

## Expanded SDF Quality Audit

```bash
adasdf_expansion_quality cube_adaptive.sdfbin --expansion global --global-resolution 64 --samples 10000
adasdf_expansion_quality cube_adaptive.sdfbin --expansion block --blocks all --block-resolution 32 --samples 10000
```

The audit reports absolute error, p95 error, sign mismatch rate, near-surface sign mismatch rate, and fallback count by comparing expanded queries against the direct model query.

Expanded query modes trade memory for query speed. GPU queries require expanded SDF layouts. The current bridge expands models by sampling; full low-rank GPU-native query remains planned work.

### Benchmark timing semantics

`total_ms` and `kernel_ms` intentionally measure different scopes. `kernel_ms` is CUDA event time for the expanded SDF kernel only. `total_ms` includes host-side output allocation, point upload, kernel synchronization, result download, CPU postprocess, and one-shot allocation/free when `--reuse-resident` is not used. Original UI CUDA numbers that report warmed kernel average time should be compared to `--kernel-only --output phi --reuse-resident` rows, not to full `total_ms` rows.

`--output phi` computes signed distance only. `--output phi,normal` computes signed distance plus finite-difference normals and is the full query primitive currently used by contact workflows. `--device-only` / `--no-download` skips D2H result download and correctness checks; it is a performance-analysis mode, not a correctness mode. The legacy `--phi-only` flag is retained as an alias for `--output phi`.

Benchmark output fields are:

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,max_abs_error,mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,fallback_rate,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
```

## Query Modes

The public query-mode API separates execution backend from expansion strategy:

```cpp
auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

adasdf::ExpansionOptions expansion;
expansion.expansion = adasdf::QueryExpansionMode::Global;
expansion.global_resolution = 64;

adasdf::QueryEngine engine(
    build.model,
    adasdf::QueryModeConfig::cudaGlobalExpanded(),
    expansion);
engine.prepare();

adasdf::BatchQueryOutput output = engine.queryBatch(points);
```

Supported mode combinations:

| Backend | Expansion | Status |
| --- | --- | --- |
| CPU | None | Direct `SDFModel` query |
| CPU | Global | Pre-expanded dense global grid |
| CPU | Block | Pre-expanded dense selected blocks |
| CUDA | Global | GPU-resident dense global grid |
| CUDA | Block | GPU-resident dense selected blocks |
| CUDA | None | Invalid: CUDA requires pre-expanded SDF data |

See `docs/query_backend_matrix.md`.

## Contact Output

AdaSDF-CL exposes contact point, contact normal, penetration depth, signed
distance, object ids, feature ids, gradients, raw contact count, reduced contact
count, and deterministic contact reduction. Stable robot-grade contact manifold
clustering, contact confidence, per-contact error estimates, and CCD remain
planned.

See `docs/contact_output_matrix.md`.

## FCL Complement Strategy

AdaSDF-CL is not a drop-in FCL replacement. The recommended positioning is:

```text
FCL-style SDF collision backend under development
```

It complements FCL-style workflows with SDF-native signed-distance queries,
penetration depth, contact normals, batch query, expanded-SDF quality audit, and
optional CUDA expanded query and an SDF-native CollisionWorld broadphase. A
true FCL fallback backend and hybrid mesh/SDF pipeline are planned, not
implemented in v1.14.0-alpha.

See `docs/fcl_complement_strategy.md` and `docs/public_positioning.md`.

## Implemented / Partial / Planned

- Implemented now: core-free demo workflow, FCL-style pair collision API,
  contact output, contact reduction, CPU/CUDA expanded query modes, benchmark
  timing semantics, ExpandedSDF quality audit, sign metrics, SVG view, and
  external CMake integration, plus STL mesh diagnostics, readiness scoring,
  safe cleanup, ASCII STL export, standalone uniform DenseSDF building,
  standalone adaptive octree/block dense SDF building, and matrix-SVD low-rank
  adaptive block compression, pure-Python CLI wrappers, and sparse SDF
  collision/contact candidate APIs, plus CPU contact-aware active block
  expansion/cache and CollisionWorld broadphase.
- Partial / experimental: demo surrogate, adaptive compressed builder preview,
  existing-core bridge, CUDA expanded query backend, block-expanded query,
  direct compressed sparse query for small point sets, and contact manifold
  behavior.
- Planned: persistent CUDA active block residency, Tucker/HOSVD compression, trained
  surrogate integration, complex mesh repair, hole filling, self-intersection
  detection, FCL fallback backend, CCD, native
  Python bindings, ROS/MoveIt, robot benchmarks, and full low-rank GPU-native
  SDF query.

See `docs/implemented_vs_planned.md` and `docs/capability_matrix.md`.

## Demo Adaptive SDFBin Format

v0.9 adds a text demo adaptive format:

```text
ADASDF_DEMO_ADAPTIVE_SDFBIN_V1
shape box
center 0 0 0
half_extent 0.5 0.5 0.5
unit m
target_near_surface_error 0.001
memory_limit_mb 64
block_expand_limit_mb 16
octree_nodes ...
blocks ...
surrogate demo_v0_9
warning demo_surrogate_not_universal_not_fully_trained
```

`SDFBinReader::read()` detects v0.9 adaptive demo magic first, then v0.8 demo magic, then falls back to the existing-core reader.

## C++ Sketch

```cpp
#include <adasdf/adasdf.h>

adasdf::DemoAdaptiveBuildRequest build_request;
build_request.use_surrogate = true;
build_request.target_near_surface_error = 1.0e-3;
build_request.memory_limit_mb = 64.0;

auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

adasdf::CollisionObject a(build.model);
adasdf::CollisionObject b(build.model);
b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

adasdf::CollisionRequest request;
request.enable_contact = true;
request.max_contacts = 8;

adasdf::CollisionResult result;
adasdf::collide(a, b, request, result);
```

`CollisionRequest::max_contacts`, `adasdf_collide --max-contacts`, and `adasdf_collide_boxes_demo --max-contacts` all cap the returned contacts. Output includes both requested and returned counts.

## Limitations

- The v0.9 surrogate is a demo / smoke / initial recommender only.
- It is not universal, not fully trained, and not an optimality guarantee.
- It is not a reliable parameter selector for arbitrary STL inputs.
- STL mesh diagnostics, readiness scoring, and safe cleanup are preflight
  utilities, not industrial mesh repair and not a full arbitrary-STL SDF builder.
- Safe cleanup does not fill holes, repair self-intersections, boolean
  reconstruct, infer units, or change model scale.
- The demo adaptive builder supports analytic boxes, not arbitrary meshes.
- Tucker/HOSVD compression remains planned.
- Deterministic build recommendation is available in v1.8, but it is not a
  universal trained model, not fully trained, and not an optimality guarantee.
- Pair collision is an approximate SDF-sampling narrow-phase.
- CollisionWorld narrowphase is sample-based SDF collision, not exact mesh-vs-mesh contact.
- CollisionWorld solver contacts are candidates, not constraints or solver impulses.
- CUDA support is limited to optional batch signed-distance/normal queries over pre-expanded global/block dense data for analytic/demo adaptive boxes.
- `CUDA + None` is intentionally rejected because CUDA compressed-direct query is not implemented.
- Selected-block CUDA benchmarks are intended for local contact-region point distributions; global uniform point clouds can make selected blocks slower or less representative.
- CUDA pair-collision acceleration, GPU-native compressed SDF query, Python bindings, FCL fallback, and FCL ABI compatibility are not implemented.

See `docs/limitations.md`.

## Docs

- `docs/demo_surrogate_recommender.md`
- `docs/demo_adaptive_builder.md`
- `docs/collision_visualization.md`
- `docs/query_modes.md`
- `docs/benchmarking.md`
- `docs/gpu_backend.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/fcl_complement_strategy.md`
- `docs/query_backend_matrix.md`
- `docs/contact_output_matrix.md`
- `docs/collision_world_scene_format.md`
- `docs/collision_world_v1_14_report.md`
- `docs/public_positioning.md`
- `docs/mesh_diagnostics.md`
- `docs/mesh_readiness.md`
- `docs/mesh_cleanup.md`
- `docs/adaptive_octree_block_builder.md`
- `docs/adaptive_block_sdfbin_format.md`
- `docs/low_rank_block_compression.md`
- `docs/compressed_block_sdfbin_format.md`
- `docs/stl_to_compressed_sdf_workflow.md`
- `docs/compression_quality_metrics.md`
- `docs/adaptive_dense_vs_compressed_sdf.md`
- `docs/github_release_draft_v1_14_0_alpha.md`
- `docs/github_release_draft_v1_7_0_alpha.md`
- `docs/low_rank_compression_v1_7_report.md`
- `docs/stl_to_adaptive_sdf_workflow.md`
- `docs/adaptive_vs_dense_sdf.md`
- `docs/stl_import_audit.md`
- `docs/github_release_draft_v1_6_0_alpha.md`
- `docs/adaptive_block_sdf_builder_v1_6_report.md`
- `docs/github_release_draft_v1_4_0_alpha.md`
- `docs/mesh_cleanup_v1_4_report.md`
- `docs/github_release_draft_v1_3_0_alpha.md`
- `docs/github_release_draft_v1_2_0_alpha.md`
- `docs/capability_audit_v1_1_1.md`
- `docs/core_free_demo_backend.md`
- `docs/alpha_status.md`
- `docs/github_release_draft_v1_1_1_alpha.md`
- `docs/query_mode_and_expansion_v1_0_1_report.md`
- `docs/cuda_benchmark_semantics_v1_0_2_report.md`
- `docs/cuda_performance_optimization_v1_0_3_report.md`
- `docs/github_release_draft_v1_0_2_alpha.md`
- `docs/github_release_draft_v1_0_2_alpha_1.md`
- `docs/github_release_draft_v1_0_3_alpha.md`

## Citation

See `CITATION.cff`. The alpha citation metadata is a placeholder and does not claim a DOI.

## License

MIT. See `LICENSE`.
