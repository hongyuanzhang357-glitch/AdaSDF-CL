# API Design

AdaSDF-CL uses a small FCL-style object/query/result vocabulary while keeping SDF-specific data visible where it matters.

## Why CollisionObject

`CollisionObject` binds an `SDFModel` to a transform, object ID, and optional user data. This lets the same model be reused by many instances and gives downstream simulation software a familiar place to attach application state.

The future implementation should map this to existing model packages and transforms:

- `adasdf::SDFModel` -> `sdf::SDFModelPackage`
- `adasdf::Transform` -> `sdf::Transform3D`
- object IDs and user data -> application-owned metadata

## Why Split CollisionRequest and CollisionResult

`CollisionRequest` describes what the user wants: contacts, maximum count, gradients, Jacobians, backend, query mode, broadphase, and narrowphase. `CollisionResult` records what happened: boolean collision state, contacts, timing, SDF query count, backend information, and minimum distance.

This split is important because contact queries may be run millions of times in simulation loops. A reusable request object avoids recreating policy state, while a result object can be cleared and reused.

## Why Keep DistanceRequest and DistanceResult

Collision and distance are related but not identical. A robotics planner may need a positive separation distance and nearest points even when objects do not collide. A simulator may only need penetration contacts. Separate request/result pairs make those workflows explicit and leave room for optimized distance-only backends.

## Why BackendType

`BackendType` makes execution placement part of the query contract:

- `CPU`: deterministic baseline and default implementation target.
- `CUDA`: optional batch query backend for large point/contact workloads.
- `Auto`: lets the library choose based on model, hardware, and query size.

CUDA must remain optional. Missing CUDA should disable CUDA targets or return a clear backend error, not break CPU users.

## v0.7 Runtime Status

AdaSDF-CL v0.7 packages the CPU SDF-sampling narrow-phase into an alpha / research-preview repository layout with version metadata, CI templates, installable CMake package config files, CLI tools, and downstream integration validation.

Current load/query path:

```text
SDFBinReader::read(path)
  -> ExistingSDFBridge
  -> sdf::ModelIO::loadBinary(path)
  -> sdf::SDFModelPackage
  -> adasdf::SDFModel
  -> SDFModel::sampleDistance(point)
  -> sdf::AdaptiveLowRankModel::queryPhi(GridInfo, point)
```

`SDFModel::sampleGradient(point)` currently uses central finite differences around `sampleDistance()`. This is a practical fallback, not a final analytic or autodiff gradient implementation.

Object-level pair queries now route through `CpuNarrowPhase` when `ExistingPairCollisionBridge` is unavailable:

```text
CollisionObject + Transform
  -> world AABB broadphase
  -> CandidatePointSampler world-space points
  -> ContactGenerator symmetric SDF queries
  -> normal orientation from object B toward object A
  -> ContactReducer deterministic merge and max_contacts cap
  -> CollisionResult / DistanceResult
```

This path is a research-preview narrow-phase. It is more stable than the v0.3 API-validation fallback, but it is still an approximate SDF-sampling method. `ExistingPairCollisionBridge` remains the future adapter surface for `sdf::PairCollisionSDF`, but it is not enabled until AdaSDF-CL can safely align native handles, source mesh ownership, and transforms. CUDA batch query, the real optional FCL adapter, Python bindings, certified contact solving, and differentiable Jacobians remain explicit TODOs.

The new adaptive builder path is:

```text
AdaptiveSDFBuilder::fromMesh(stl_path, BuildOptions)
  -> ExistingBuilderBridge
  -> sdf::readStl(path)
  -> sdf::OctreeSDF::build(mesh, OctreeSDFOptions)
  -> sdf::AdaptiveBlockCompressor::build(octree, AdaptiveCompressionOptions)
  -> sdf::makeModelMetadata(...)
  -> sdf::ModelIO::saveBinary(temp_path, package)
  -> ExistingSDFBridge::loadExistingSDFBin(temp_path)
  -> adasdf::SDFModel
```

`SDFBinWriter::write(path, model)` delegates to a native model handle when the loaded or built model can write the existing-core `.sdfbin` package. It then reloads the output through `SDFBinReader::read()` as a quick round-trip validation.

`AdaptiveSDFBuilder::fromOBJ()` and the `MeshModel` overload are reserved API surfaces in v0.4+. They report unsupported input clearly instead of pretending that OBJ or in-memory mesh construction has been bridged.

## BuildOptions and Surrogate Terms

`BuildOptions` now carries both user-facing AdaSDF-CL controls and compatibility names used by DASH-SDF recommendation experiments:

- Accuracy and memory: `near_surface_error`, `far_field_error`, `max_memory_mb`, `block_expand_limit_mb`.
- Adaptive compression: `max_octree_level`, `min_octree_level`, `max_rank`, `enable_compression`, `compression_method`.
- DASH-SDF terminology: `tau_near_abs`, `memory_limit_mb`, `block_memory_limit_mb`, `use_surrogate_recommendation`, `surrogate_top_k`.
- Existing-core bridge parameters: block sizes, ranks, ghost cells, sign-ray count, BVH leaf size, and near-band policy.

The bridge maps the absolute near-surface target into the existing core's error-over-spacing parameter with a practical lower bound. This is a compatibility mapping, not yet a mathematically exact absolute-error controller.

`SurrogateRecommender` is deliberately conservative in v0.4+. It exposes the future API but returns `credible=false` when no compiled surrogate backend is present.

## Collision Request and Result Statistics

`CollisionRequest` includes advanced controls for the CPU narrow-phase:

- `enable_contact_reduction`
- `candidate_grid_resolution`
- `max_candidate_points`
- `near_surface_band`
- `contact_merge_tolerance`
- `normal_merge_cosine`
- `symmetric_query`

`QueryMode::Fast`, `Balanced`, and `Accurate` map to default sampling densities of 4/512, 8/4096, and 16/20000. Explicit request fields can override those defaults.

`CollisionResult` and `DistanceResult` expose:

- `numCandidatePoints()`
- `numRawContacts()`
- `numReducedContacts()`
- `numSDFQueries()`
- `backendInfo()`
- `methodInfo()`

`distance()` returns an AABB lower-bound distance when world AABBs are separated. When AABBs overlap, it returns the best signed value found by symmetric candidate-point SDF sampling. This is useful for research-preview ranking and contact experiments, but it is not a strict global closest-distance solver.

## Why QueryMode

`QueryMode` lets users express an accuracy/performance tradeoff without selecting internal algorithms directly:

- `Fast`: point sampling plus fast SDF lookup; useful for RL, large batches, and approximate real-time contact.
- `Balanced`: default mode using broadphase, adaptive narrowphase, and compressed SDF query.
- `Accurate`: higher-precision contact search for engineering analysis and validation.

## Beginner and Advanced Users

Beginner users should be able to load a `.sdfbin`, create two `CollisionObject`s, and call `collide()`. Advanced users should be able to control block compression, backend choice, query mode, gradient/Jacobian outputs, and batched execution.

The public API should therefore keep common defaults simple while storing detailed options in explicit option structs.

## Future Bindings and Integrations

The public API should be binding-friendly:

- Python: expose models, transforms, requests, results, and NumPy-compatible batch buffers.
- MATLAB: expose file I/O and matrix-friendly batched query functions.
- ROS: wrap `CollisionObject` in robot link or planning scene adapters.
- MuJoCo and Chrono: provide contact generation callbacks that consume `Contact`.
- Custom MBD software: use contact-only `.sdfbin` assets and stable C ABI or Python bridge where needed.

Adapters should depend on the stable `adasdf` API rather than internal `sdf::` classes.
