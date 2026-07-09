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
- Alpha contact-focused narrow-band SDF construction for collision-oriented
  use cases.
- End-to-end contact-band benchmark speedup when contact-band quality and
  coverage gates pass.
- Fast active block lookup and cache slot mapping for CPU active block runtime
  queries.
- Block lookup benchmark reporting speedup and consistency gates together.
- Stable backend JSON contract and versioned schemas for Studio/automation.
- Build profile, progress JSONL, and timeout reporting for dense, adaptive,
  adaptive-compression, and compressed SDF build workflows.
- Adaptive build sample cache for reducing repeated build-time SDF queries with
  no-cache reference benchmarking.
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
- Universal speedup for all STL models.
- Global full-field SDF reconstruction acceleration from contact-band sampling.
- Marker-excluded speedup as end-to-end performance.
- GPU hash table in v1.17.
- Hashing the eight trilinear interpolation nodes.
- Fast lookup speedup without mismatch and phi-difference checks.
- Treating no-collision or no-candidate JSON status codes as crashes.
- Assuming v1.17.1 JSON schemas imply a `.sdfbin` format change.
- Claiming cache speedup without no-cache quality comparison.
- Claiming global cache is always faster or always lower memory.
- Industrial certification from synthetic fixture benchmarks.

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
guarded transition/far-field prediction, and exact fallback. v1.16.0-alpha.3
adds contact-focused narrow-band construction for collision-oriented use cases,
with contact-band exact sampling, relaxed far-field interpolation, quality and
coverage gates, and end-to-end timing semantics. v1.17 adds fast active block
lookup metadata and cache slot maps for CPU active block runtime queries while
keeping block-internal trilinear interpolation as direct regular-grid indexing.
v1.17.1 adds stable backend JSON schemas, model export CLIs, collision and
benchmark JSON contract output, and build profile/progress/timeout reporting
for dense, adaptive, adaptive-compression, and compressed SDF build workflows
without changing SDF algorithms or `.sdfbin` formats. v1.17.2 adds build-time
sample caches and no-cache reference benchmarking for repeated adaptive/contact
band SDF queries without changing `.sdfbin` formats or runtime query APIs. It
does not
fill holes, repair self-intersections, implement Tucker/HOSVD compression,
claim a universal trained recommender, guarantee optimal parameters, claim
GPU-native compressed query, provide GPU BVH, provide FCL fallback, claim
global full-field SDF reconstruction acceleration from contact-band sampling,
provide exact mesh-vs-mesh world contact, implement a v1.17 GPU hash table,
provide a full contact solver, or provide a native pybind11 binding.
