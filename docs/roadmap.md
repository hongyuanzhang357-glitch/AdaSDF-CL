# Roadmap

## v0.1 Interface Preview

- README
- C++ header skeleton
- `.sdfbin` documentation
- examples
- architecture docs

## v0.2 Core CPU Query

- load existing `.sdfbin` through `sdf::ModelIO`
- expose loaded metadata and AABB through `SDFModel`
- single-point signed-distance query through `sdf::AdaptiveLowRankModel::queryPhi`
- finite-difference gradient fallback
- working load/query example
- basic runtime tests

Deferred from v0.2:

- object-object `distance()`
- object-object `collide()`
- analytic low-rank gradient
- CUDA batched query
- FCL adapter execution

## v0.3 Pair Query API Validation

- `CollisionObject` transform-aware world AABB
- `DistanceRequest` / `DistanceResult` usable runtime state
- `CollisionRequest` / `CollisionResult` / `Contact` populated by pair query
- `distance(obj_a, obj_b, ...)` CPU fallback
- `collide(obj_a, obj_b, ...)` CPU fallback with contacts
- command-line pair-query and FCL-style examples
- graceful real `.sdfbin` tests

Deferred from v0.3:

- direct `sdf::PairCollisionSDF` bridge
- high-precision adaptive narrowphase
- CUDA batched pair query
- real optional FCL dependency adapter
- differentiable Jacobians

## v0.4 Adaptive Builder Bridge

- STL mesh input through `ExistingBuilderBridge`
- `AdaptiveSDFBuilder::fromMesh()` connected to the existing core for STL
- `SDFBinWriter::write()` connected for existing-core-backed models
- `adasdf_build` CLI with build options and reload validation
- DASH-SDF `SurrogateRecommender` placeholder with no fake predictions
- builder, writer, surrogate, and round-trip tests

Deferred from v0.4:

- OBJ mesh loader bridge
- in-memory `MeshModel` adaptive builder
- exact absolute-error controller in the old adaptive compression policy
- real DASH-SDF runtime integration
- contact-only `.sdfbin` writer/reader

## v0.5 CPU Narrow-Phase and Contact Reduction

- `CpuNarrowPhase` named research-preview backend
- `CandidatePointSampler` for transformed world-space candidate points
- `ContactGenerator` for symmetric SDF queries and normal stabilization
- `ContactReducer` for deterministic merge and `max_contacts` enforcement
- improved `CollisionResult` and `DistanceResult` statistics
- contact reduction demo and CPU narrow-phase tests

Deferred from v0.5:

- CUDA batched narrow-phase
- strict global closest-distance solver
- certified industrial contact solver
- direct `sdf::PairCollisionSDF` bridge
- FCL ABI compatibility

## v0.6 Alpha Release Hardening

- version metadata
- public-facing README
- release notes and changelog
- contribution and conduct docs
- CI templates
- repository clean checks
- alpha validation script
- local git initialization and initial commit

## v0.7 Install / Package and External Integration

- validate `find_package(AdaSDFCL CONFIG REQUIRED)`
- add a standalone downstream CMake example
- validate install tree
- add package config tests
- refine public API naming
- prepare real GitHub remote and release tag

## v0.8 Python Binding Preview

- define Python package layout
- expose model load/query basics
- expose build options and collision request/result wrappers
- keep native runtime optional and documented

## v0.9 CUDA Batched Query Preview

- prototype CUDA batched point/query path
- add optional GPU test gating
- keep CPU path as default
- document hardware and driver requirements

## v1.0 Stable Research Release

- stable API
- documentation
- examples
- CI
- benchmark suite
- citation release
