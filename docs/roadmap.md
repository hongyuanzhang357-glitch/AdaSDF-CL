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

## v0.3 Pair Query API Validation

- `CollisionObject` transform-aware world AABB
- `DistanceRequest` / `DistanceResult` usable runtime state
- `CollisionRequest` / `CollisionResult` / `Contact` populated by pair query
- `distance(obj_a, obj_b, ...)` CPU fallback
- `collide(obj_a, obj_b, ...)` CPU fallback with contacts
- command-line pair-query and FCL-style examples
- graceful real `.sdfbin` tests

## v0.4 Adaptive Builder Bridge

- STL mesh input through `ExistingBuilderBridge`
- `AdaptiveSDFBuilder::fromMesh()` connected to the existing core for STL
- `SDFBinWriter::write()` connected for existing-core-backed models
- `adasdf_build` CLI with build options and reload validation
- DASH-SDF `SurrogateRecommender` placeholder with no fake predictions
- builder, writer, surrogate, and round-trip tests

## v0.5 CPU Narrow-Phase and Contact Reduction

- `CpuNarrowPhase` named research-preview backend
- `CandidatePointSampler` for transformed world-space candidate points
- `ContactGenerator` for symmetric SDF queries and normal stabilization
- `ContactReducer` for deterministic merge and `max_contacts` enforcement
- improved `CollisionResult` and `DistanceResult` statistics
- contact reduction demo and CPU narrow-phase tests

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
- install CLI tools
- add CPack source archive preparation

## v0.7.0-alpha.1 CI-Fixed Pre-Release

- preserve the original `v0.7.0-alpha` preflight tag for traceability
- publish `v0.7.0-alpha.1` as the recommended first public pre-release
- include the Ubuntu install validation portability fix
- keep core algorithms and public API unchanged

## v0.7.0-alpha.2 External Test Documentation Hotfix

- document the external collision test verdict as PARTIAL
- clarify core-free public builds versus existing-core enhanced builds
- state that clone-only end-to-end public collision demos are planned for a later version
- keep core algorithms, public API, and CMake install/export behavior unchanged

## v0.8.0-alpha Core-Free Standalone Demo Backend

- add `AnalyticSDFModel` for axis-aligned box SDF queries
- add demo `.sdfbin` text format with magic `ADASDF_DEMO_SDFBIN_V1`
- add `adasdf_make_demo_box`
- support demo `.sdfbin` in `adasdf_info`, `adasdf_query`, and `adasdf_collide`
- add core-free collision example and tests
- validate clone-only build/install/query/collide workflow
- document that this is a demo backend, not the full adaptive compressed SDF builder

## v0.9 Python Binding Preview

- define Python package layout
- expose model load/query basics
- expose build options and collision request/result wrappers
- keep native runtime optional and documented

## v1.0 Stable Research Release

- stable API
- documentation
- examples
- CI
- benchmark suite
- citation release
- production-quality public adaptive backend plan
