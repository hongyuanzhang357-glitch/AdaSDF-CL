# AdaSDF-CL v0.2 SDFBin Integration Report

## 1. Goal

Connect AdaSDF-CL to the existing `.sdfbin` load and query path so the library can load real parent-project SDF assets and perform a minimal point signed-distance and gradient query loop.

## 2. Completed Work

- Added `ExistingSDFBridge`.
- Added runtime implementation files for `SDFModel`, `SDFBinReader`, basic query result classes, backend stubs, and adapter stubs.
- Added `adasdf_cl_runtime` CMake target.
- Added `ADASDF_CL_USE_EXISTING_CORE`.
- Compiled a non-UI, non-CUDA, non-FCL existing core static library when parent dependencies are available.
- Connected `SDFBinReader::read()` to `sdf::ModelIO::loadBinary()`.
- Connected `SDFModel::sampleDistance()` to `sdf::AdaptiveLowRankModel::queryPhi()`.
- Added finite-difference `SDFModel::sampleGradient()` and `sampleNormal()`.
- Updated the load/query example into a real executable.
- Added real-load and query tests with graceful skip behavior when fixtures or the bridge are unavailable.

## 3. Added Files

- `adasdf_cl/include/adasdf/io/ExistingSDFBridge.h`
- `adasdf_cl/src/io/ExistingSDFBridge.cpp`
- `adasdf_cl/src/io/SDFBinReader.cpp`
- `adasdf_cl/src/io/SDFBinWriter.cpp`
- `adasdf_cl/src/io/ContactOnlySDFBin.cpp`
- `adasdf_cl/src/geometry/SDFModel.cpp`
- `adasdf_cl/src/geometry/Transform.cpp`
- `adasdf_cl/src/geometry/MeshModel.cpp`
- `adasdf_cl/src/compression/CompressedSDF.cpp`
- `adasdf_cl/src/backend/Backend.cpp`
- `adasdf_cl/src/backend/GpuBackend.cpp`
- `adasdf_cl/src/generation/SDFBuilder.cpp`
- `adasdf_cl/src/generation/AdaptiveSDFBuilder.cpp`
- `adasdf_cl/src/query/CollisionObject.cpp`
- `adasdf_cl/src/query/CollisionResult.cpp`
- `adasdf_cl/src/query/DistanceResult.cpp`
- `adasdf_cl/src/query/QueryAPI.cpp`
- `adasdf_cl/src/adapters/FCLAdapter.cpp`
- `adasdf_cl/tests/test_sdfbin_real_load.cpp`
- `adasdf_cl/tests/test_sdfmodel_query.cpp`
- `adasdf_cl/docs/sdfbin_integration_audit.md`
- `adasdf_cl/docs/adasdf_cl_v0_2_sdfbin_integration_report.md`

## 4. Modified Files

- `adasdf_cl/CMakeLists.txt`
- `adasdf_cl/README.md`
- `adasdf_cl/examples/02_load_sdfbin_and_query.cpp`
- `adasdf_cl/include/adasdf/adasdf.h`
- `adasdf_cl/include/adasdf/geometry/SDFModel.h`
- `adasdf_cl/include/adasdf/io/SDFBinReader.h`
- `adasdf_cl/docs/api_design.md`
- `adasdf_cl/docs/sdfbin_format.md`
- `adasdf_cl/docs/roadmap.md`

## 5. Existing SDFBin Load Path

```text
sdf::ModelIO::inspectBinaryHeader(path)
sdf::ModelIO::loadBinary(path)
    -> sdf::SDFModelPackage
        -> metadata
        -> adaptiveModel
```

AdaSDF-CL uses this path unchanged. The binary format was not modified.

## 6. SDFBinReader Implementation

`adasdf::SDFBinReader::read(path)` now:

1. checks for empty path
2. checks file existence
3. calls `ExistingSDFBridge::loadExistingSDFBin(path)`
4. converts thrown errors into `std::runtime_error` with file context

The bridge uses `sdf::ModelIO::inspectBinaryHeader()` for format validation before loading.

## 7. SDFModel Current Capabilities

Implemented:

- `isValid()`
- `boundingBox()`
- `memoryFootprintBytes()`
- `nearSurfaceError()`
- `metadata()`
- `queryBackendAvailable()`
- `sampleDistance(point)`
- `sampleGradient(point)`
- `sampleNormal(point)`
- `sample(point)`

Coordinates are currently interpreted in the loaded model/root coordinate system. Object transform handling is preserved in the public API but is not yet wired into model sampling.

## 8. Query API Status

Real:

- single-model point signed-distance query through loaded `.sdfbin`
- finite-difference gradient query
- normal query from finite-difference gradient

Still TODO:

- `distance(obj_a, obj_b, request, result)`
- `collide(obj_a, obj_b, request, result)`
- `collideBatch(...)`
- CUDA backend execution
- FCL adapter execution

## 9. Gradient Source

Gradient is finite difference in v0.2.

It is not an analytic low-rank gradient and not an autodiff/Jacobian implementation. The finite-difference step is based on half the stored fine-grid spacing where available.

## 10. Test Results

Commands run:

```text
cmake -S adasdf_cl -B build/adasdf_cl-v0_2 -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=ON
cmake --build build/adasdf_cl-v0_2 --config Debug
ctest --test-dir build/adasdf_cl-v0_2 -C Debug --output-on-failure
```

Result:

```text
6/6 tests passed
```

The passing tests include:

- `test_sdfbin_real_load`
- `test_sdfmodel_query`

## 11. Example Run

Command run:

```text
build/adasdf_cl-v0_2/Debug/adasdf_load_sdfbin_and_query.exe models/ui_outputs/auto20_validation_closed_cube_v2/closed_cube_demo_svd_L4.sdfbin
```

Key output:

```text
Valid: true
AABB min: -1.35 -1.35 -1.35
AABB max: 1.35 1.35 1.35
Memory footprint: 7368 bytes
Near-surface error: 0.0971102
Backend: existing sdf::AdaptiveLowRankModel CPU query
Signed distance at center: -0.800981
```

Windows console output showed mojibake for Chinese path segments, but the file loaded and queried successfully.

## 12. Current Limitations

- `SDFBinWriter` remains unimplemented in AdaSDF-CL.
- Builder APIs remain preview only.
- Object-object distance/collision is not wired yet.
- Finite-difference gradient can be noisy near discontinuities or block boundaries.
- `adasdf_cl` currently compiles a parent C++ core static library when using the bridge, which is functional but heavier than a future installed/exported core target.
- CUDA and FCL are not required and not executed.

## 13. Next Round

Recommended v0.3 task:

```text
CollisionObject + distance() + collide() real narrowphase query
```

Use `sdf::PairCollisionSDF` or an equivalent parent module to implement:

```cpp
distance(obj_a, obj_b, request, result)
collide(obj_a, obj_b, request, result)
```

The bridge should continue to isolate old `sdf::` types from public `adasdf::` headers.
