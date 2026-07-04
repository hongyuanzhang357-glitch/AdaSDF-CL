# Public Positioning

AdaSDF-CL is an FCL-style adaptive SDF collision research library. It is
currently best understood as a complementary SDF backend, not a full FCL
replacement.

## Recommended Wording

- FCL-style SDF collision backend.
- Research-preview adaptive SDF collision library.
- Signed distance, normal, and penetration query.
- Batch query and optional CUDA expanded-SDF path.
- Expanded-SDF quality audit and sign-risk metrics.
- STL mesh diagnostics preflight for import readiness.
- SDF build readiness scoring and repair suggestions.
- Safe mesh cleanup and clean ASCII STL export.
- Standalone uniform DenseSDF builder.
- Standalone adaptive octree/block SDF builder with dense per-block values.
- Matrix-SVD compressed adaptive block SDF builder.
- Compression quality audit and dense fallback blocks.
- Experimental deterministic build recommender for DenseSDF, AdaptiveBlockSDF,
  and CompressedAdaptiveBlockSDF parameters.
- Pure-Python subprocess wrapper for installed CLI tools.
- Sparse point-to-SDF collision query.
- Collision-only early-exit query.
- Sample-radius point/sphere proxy collision.
- Top-K contact candidate reduction for hard-contact budgets.
- Solver-aware contact candidate stabilization with deterministic patch
  clustering, contact budgets, and solver-ready CSV/Markdown/JSON-like export.
- CollisionWorld broadphase for multiple SDF objects with simple CSV scenes,
  deterministic AABB filtering, sample-based sparse collision, and
  solver-ready contact candidate export.
- Strict JSON/CSV reproducibility reports for build, query, benchmark, and
  validation automation.
- Contact-aware CPU active block expansion/cache for compressed SDF runtime
  memory savings.
- Optional CPU BVH acceleration for STL-to-SDF builder sampling.
- Deterministic multi-threaded builder sampling.
- Experimental hierarchical adaptive sampling with quality guard for adaptive
  and compressed SDF construction.
- Adaptive builder preview for future Tucker/trained-surrogate/GPU compressed
  stages.

## Avoid Wording

- Industrial-grade replacement for FCL.
- Certified collision engine.
- Universal adaptive STL-to-SDF solution.
- Tucker/HOSVD compression.
- GPU-native compressed query.
- Automatic mesh repair tool.
- Automatic hole filling or self-intersection repair tool.
- Industrial mesh certification tool.
- Guaranteed optimal surrogate recommender.
- Universal trained build recommender.
- Fully trained recommendation model.
- Complete GPU contact solver.
- GPU BVH.
- Native Python binding.
- Native pybind11 binding.
- Full contact solver.
- Complete contact manifold from sparse candidates.
- Exact mesh-vs-mesh CollisionWorld contact.
- CollisionWorld as FCL fallback.
- CollisionWorld as a scene graph.
- Strict reports as solver constraints or mesh certification.
- GPU-native compressed direct query in v1.9.
- CUDA active block cache in v1.10.
- BVH acceleration as a GPU query backend.

## Short Description

AdaSDF-CL complements FCL-style workflows by adding SDF-native signed-distance
queries, penetration-depth estimates, contact normals, batch query tooling,
expanded-SDF quality audits, optional CUDA expanded-query paths, STL mesh
diagnostics, SDF build readiness scoring, safe mesh cleanup, and a public
uniform DenseSDF builder, a public adaptive octree/block SDF builder, and a
public matrix-SVD compressed adaptive block SDF builder. v1.8 adds a
deterministic recommend-then-build parameter recommendation path. v1.8.1 adds
a pure-Python subprocess wrapper for the installed CLI tools. v1.9 adds sparse
point-to-SDF collision, collision-only early exit, sample-radius proxy
collision, and Top-K contact candidate reduction. v1.10 adds CPU active block
expansion/cache for local compressed SDF query working sets. v1.12 adds an
optional CPU TriangleBVH builder acceleration path and deterministic
multi-threaded builder sampling. v1.13 adds solver-aware contact candidate
stabilization for external hard-contact simulation workflows. v1.14 adds
CollisionWorld broadphase and multi-object sample-based SDF collision. v1.15
adds strict JSON/CSV reproducibility reports for automation. v1.16 adds
experimental hierarchical adaptive sampling with near-surface exact sampling,
guarded transition/far-field prediction, and exact fallback. It does not
fill holes, repair self-intersections, implement Tucker/HOSVD compression,
claim a universal trained recommender, guarantee optimal parameters, claim
GPU-native compressed query, provide GPU BVH, provide FCL fallback, provide
exact mesh-vs-mesh world contact, provide a full contact solver, or provide a
native pybind11 binding.
