# SDFBin Integration Audit

## Relevant Files Found

- `include/sdf/ModelIO.h`, `src/ModelIO.cpp`
- `include/sdf/SDFModelPackage.h`, `src/SDFModelPackage.cpp`
- `include/sdf/AdaptiveLowRankModel.h`, `src/AdaptiveLowRankModel.cpp`
- `include/sdf/LowRankBlock.h`
- `include/sdf/LowRankCompressionStats.h`, `src/LowRankCompressionStats.cpp`
- `include/sdf/MatrixSVDCompressor.h`, `src/MatrixSVDCompressor.cpp`
- `include/sdf/TuckerCompressor.h`, `src/TuckerCompressor.cpp`
- `include/sdf/TTCompressor.h`, `src/TTCompressor.cpp`
- `include/sdf/OctreeSDF.h`, `src/OctreeSDF.cpp`
- `include/sdf/AdaptiveBlockCompressor.h`, `src/AdaptiveBlockCompressor.cpp`
- `include/sdf/PairCollisionSDF.h`, `src/PairCollisionSDF.cpp`
- `include/sdf/cuda/CudaAllPointsSDFContact.h`, `src/cuda/CudaAllPointsSDFContact.cu`

## Existing SDFBin Load Flow

The existing binary reader is `sdf::ModelIO::loadBinary(path)`.

It reads:

1. magic header `SDFLRM01`
2. binary format version
3. `SDFModelMetadata`
4. `AdaptiveCompressionStats`
5. micro-block map
6. `AdaptiveBlockInfo` records
7. compressed `LowRankBlock` records

It returns:

```cpp
sdf::SDFModelPackage
```

The package contains:

```cpp
SDFModelMetadata metadata;
AdaptiveLowRankModel adaptiveModel;
```

## Existing SDFBin Save Flow

The existing writer is `sdf::ModelIO::saveBinary(path, package)`.

It writes the same model package structure and is used by the parent project after adaptive low-rank model construction. AdaSDF-CL v0.2 does not change this format and does not implement a new writer.

## Data Structure Relationship

```text
sdf::ModelIO::loadBinary
    -> sdf::SDFModelPackage
        -> sdf::SDFModelMetadata
        -> sdf::AdaptiveLowRankModel
            -> vector<sdf::LowRankBlock>
            -> vector<sdf::AdaptiveBlockInfo>
            -> micro-block lookup map
```

AdaSDF-CL wraps this as:

```text
adasdf::SDFBinReader::read
    -> adasdf::ExistingSDFBridge
    -> adasdf::SDFModel
        -> adasdf::SDFMetadata
        -> adasdf::AABB
        -> adasdf::SDFModel::NativeHandle
```

The native handle owns a `sdf::SDFModelPackage` internally and exposes only distance-query methods to `adasdf::SDFModel`.

## Existing Single-point Query

`sdf::AdaptiveLowRankModel::queryPhi(const GridInfo&, const Vec3&)` provides single-point signed-distance query over the loaded compressed adaptive model. This is directly reusable for v0.2.

## Existing Gradient / Normal Query

The parent project has `sdf::SDFNormal` and contact-query code that computes normals in some paths, but there is no single stable loaded-model gradient API matching `SDFModel::sampleGradient(point)`.

AdaSDF-CL v0.2 therefore uses central finite differences over `sampleDistance()`:

```text
[phi(x+h)-phi(x-h), phi(y+h)-phi(y-h), phi(z+h)-phi(z-h)] / (2h)
```

The step defaults to half the stored fine-grid spacing, with a small fallback if metadata is incomplete.

## Block / Octree / Low-rank Metadata

The existing model package stores:

- global root AABB
- fine-grid cell/node counts
- fine-grid spacing
- max octree level and build options
- final/active/sign-change/near-surface block counts
- low-rank memory estimate
- dense memory estimate
- compression ratio
- per-block range, resolution, rank, and reconstruction statistics

AdaSDF-CL v0.2 maps the stable subset into `adasdf::SDFMetadata` and `adasdf::SDFBlockMetadata`.

## UI / Visualization Dependencies

The reader and query path used in v0.2 do not depend on the Python UI, viewers, or UI slice caches. The `adasdf_cl` bridge builds a CPU C++ core subset from parent sources and excludes CUDA, FCL, Python, and UI modules.

## Directly Reusable Functions

- `sdf::ModelIO::inspectBinaryHeader`
- `sdf::ModelIO::loadBinary`
- `sdf::AdaptiveLowRankModel::queryPhi`
- `sdf::AdaptiveLowRankModel::blocks`
- `sdf::LowRankBlock` metadata and stats

## Not Directly Reused in v0.2

- `sdf::ModelIO::saveBinary`: writer remains future work for AdaSDF-CL.
- `sdf::PairCollisionSDF`: object-object query is reserved for the next round.
- CUDA query modules: optional GPU backend is reserved for a later round.
- FCL adapter: remains optional and not required for v0.2.
- Python UI and report-generation workflows: not part of the bridge.

## Adopted Integration Plan

1. Keep `adasdf::cl` as the first-pass interface target.
2. Add `adasdf_cl_runtime` for v0.2 implementation code.
3. Add `ExistingSDFBridge` as the only place that includes old `sdf::` headers.
4. Let `SDFBinReader::read()` validate path existence and call the bridge.
5. Store the native package behind `SDFModel::NativeHandle` so public headers do not expose old core types.
6. Add tests and a runnable example using existing `.sdfbin` files discovered under `models/`.
