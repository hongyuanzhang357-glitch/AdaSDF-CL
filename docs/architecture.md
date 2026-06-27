# AdaSDF-CL Architecture

AdaSDF-CL is organized as a layered collision-query pipeline:

```text
Mesh / CAD / STL
      ->
Adaptive SDF Generation
      ->
Octree / Block Refinement
      ->
Low-rank / SVD Compression
      ->
SDFBin / Contact-only SDFBin
      ->
CPU / CUDA Query Backend
      ->
Collision / Distance / Contact API
      ->
Robotics / MBD / CAE / RL / Simulation Software
```

## Mesh / CAD / STL

This layer imports source geometry and produces a clean triangle mesh. The existing project already has `sdf::StlReader`, `sdf::Mesh`, `sdf::AABB`, triangle distance, ray-triangle, and BVH code that should be reused instead of rewritten.

## Adaptive SDF Generation

This layer samples signed distance values around the mesh and owns sign convention, sampling tolerance, and mesh-to-grid metadata. The future `adasdf::SDFBuilder` and `adasdf::AdaptiveSDFBuilder` should connect to `sdf::SignedDistance` and `sdf::OctreeSDF`.

In v0.4+, `AdaptiveSDFBuilder::fromMesh()` is connected for STL files through `ExistingBuilderBridge`. The bridge reuses the parent core directly: STL loading, octree construction, adaptive block compression, metadata creation, and binary save/load validation. OBJ and in-memory `MeshModel` construction remain reserved surfaces with explicit unsupported errors.

## Octree / Block Refinement

This layer places resolution where contact queries need it. It should preserve block IDs, local coordinate ranges, fine-grid spacing, and near-surface tags. The existing `sdf::OctreeSDF`, `sdf::BlockPartition`, `sdf::DenseBlockGenerator`, and `sdf::DenseBlockCache` are the main implementation candidates.

## Low-rank / SVD Compression

This layer compresses dense SDF blocks into Matrix SVD, Tucker, or Tensor Train representations. The public `adasdf::CompressedSDF` and `adasdf::LowRankBlock` types are interface-level containers; implementation should reuse `sdf::LowRankBlock`, `sdf::MatrixSVDCompressor`, `sdf::TuckerCompressor`, `sdf::TTCompressor`, `sdf::AdaptiveBlockCompressor`, and `sdf::AdaptiveLowRankModel`.

## SDFBin / Contact-only SDFBin

This layer persists compressed models. Full `.sdfbin` should preserve enough metadata for reconstruction, diagnostics, and compatibility. Contact-only `.sdfbin` should be a smaller deployable asset for engineering contact queries, with no UI caches or temporary benchmark data.

`SDFBinWriter::write()` now works for models backed by an existing-core native package. It writes through `sdf::ModelIO::saveBinary()` and immediately reloads the result through the public reader to catch broken outputs early.

## CPU / CUDA Query Backend

This layer evaluates signed distance, gradient, contact, and batch queries. CPU is the baseline. CUDA is optional and must not make the whole library unavailable when absent. Existing implementation candidates include `sdf::PairCollisionSDF`, `sdf::AllPointsSDFContact`, `sdf::StreamingBVHSDFContact`, `sdf::CudaGlobalDenseSDF`, and `sdf::CudaAllPointsSDFContact`.

In v0.6, the working CPU pair-query path remains `CpuNarrowPhase`, a research-preview SDF-sampling narrow-phase used by `adasdf::collide()` and `adasdf::distance()` when `ExistingPairCollisionBridge` is unavailable. It performs world AABB broadphase, transformed candidate-point sampling, symmetric target SDF queries, normal stabilization, and deterministic contact reduction. `ExistingPairCollisionBridge` remains the reserved replacement point for the parent project's `sdf::PairCollisionSDF`; it remains disabled until the AdaSDF-CL model wrapper can safely expose native model package state, mesh ownership, and required transforms.

In v0.4+, the builder bridge also runs on CPU only. CUDA remains a query/build extension point, not a required dependency.

## DASH-SDF Surrogate Layer

The DASH-SDF surrogate layer is represented by `SurrogateRecommender` and additional `BuildOptions` fields. It is intentionally a placeholder in this repository: validation-card metrics are documented, but no Python runtime, joblib model, raw dataset, or large generated artifact is copied into AdaSDF-CL. When unavailable, the API returns a non-credible recommendation object with an explanatory note.

## Collision / Distance / Contact API

This layer exposes stable user-facing types: `CollisionObject`, `CollisionRequest`, `CollisionResult`, `DistanceRequest`, `DistanceResult`, and `Contact`. It should hide the underlying query path and make CPU/GPU and Fast/Balanced/Accurate modes explicit.

`CollisionObject::getAABB()` transforms the local model bounds into world coordinates using the current rigid transform. This keeps the broadphase behavior aligned with FCL-style object instances even when the same loaded `SDFModel` is reused multiple times.

## Application Integration

Robotics, MBD, CAE, RL, differentiable simulation, and engineering software should use AdaSDF-CL through the stable API, not through internal octree or compression classes. Adapters can then target Python, MATLAB, ROS, MuJoCo, Chrono, and FCL-style projects without coupling those applications to internal data structures.
