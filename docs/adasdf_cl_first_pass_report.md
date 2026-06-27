# AdaSDF-CL First-pass Report

## 1. Goal

Create a non-destructive open-source library skeleton for AdaSDF-CL: a professional README, CMake entry point, C++ public API headers, examples, format/design docs, and audit reports.

## 2. Added Files

- `adasdf_cl/README.md`
- `adasdf_cl/LICENSE`
- `adasdf_cl/CMakeLists.txt`
- `adasdf_cl/include/adasdf/adasdf.h`
- `adasdf_cl/include/adasdf/config.h`
- `adasdf_cl/include/adasdf/geometry/*`
- `adasdf_cl/include/adasdf/generation/*`
- `adasdf_cl/include/adasdf/compression/*`
- `adasdf_cl/include/adasdf/query/*`
- `adasdf_cl/include/adasdf/backend/*`
- `adasdf_cl/include/adasdf/io/*`
- `adasdf_cl/include/adasdf/adapters/*`
- `adasdf_cl/examples/01_build_adaptive_sdf.cpp`
- `adasdf_cl/examples/02_load_sdfbin_and_query.cpp`
- `adasdf_cl/examples/03_collision_between_two_objects.cpp`
- `adasdf_cl/examples/04_batched_gpu_query.cpp`
- `adasdf_cl/examples/05_fcl_style_api.cpp`
- `adasdf_cl/python/README.md`
- `adasdf_cl/python/adasdf/__init__.py`
- `adasdf_cl/docs/architecture.md`
- `adasdf_cl/docs/api_design.md`
- `adasdf_cl/docs/sdfbin_format.md`
- `adasdf_cl/docs/contact_only_sdfbin_format.md`
- `adasdf_cl/docs/fcl_compatibility.md`
- `adasdf_cl/docs/differentiable_contact.md`
- `adasdf_cl/docs/gpu_backend.md`
- `adasdf_cl/docs/roadmap.md`
- `adasdf_cl/docs/adasdf_cl_audit_report.md`
- `adasdf_cl/docs/adasdf_cl_first_pass_report.md`
- `adasdf_cl/tests/test_sdf_io.cpp`
- `adasdf_cl/tests/test_collision_query.cpp`
- `adasdf_cl/tests/test_distance_query.cpp`
- `adasdf_cl/tests/test_contact_only_sdfbin.cpp`
- `adasdf_cl/tools/adasdf_build.cpp`
- `adasdf_cl/tools/adasdf_compress.cpp`
- `adasdf_cl/tools/adasdf_query.cpp`
- `adasdf_cl/tools/adasdf_benchmark.cpp`
- `adasdf_cl/src/*/.gitkeep`

## 3. Modified Files

None. The existing project was left intact.

## 4. Files Not Modified but Recommended for Later Handling

- Root `README.md`: convert from stage log to concise project README or keep as legacy development notes.
- Root `CMakeLists.txt`: later add install/export rules and separate public/private dependencies.
- `include/sdf` and `src`: gradually wrap stable modules into `adasdf` public APIs.
- `build/`, `.venv-sdf-ui/`, `__pycache__`: exclude from an open-source repository.
- `reports/`, `models/`, `paper_package/`: split into examples, reproducibility artifacts, or release assets.

## 5. Current AdaSDF-CL Interface Overview

- Geometry: `Vector3`, `Matrix3`, `MatrixX`, `AABB`, `Transform`, `MeshModel`, `CollisionGeometry`, `SDFModel`.
- Generation: `BuildOptions`, `SDFBuilder`, `AdaptiveSDFBuilder`.
- Compression: `CompressionOptions`, `LowRankBlock`, `CompressedSDF`.
- Query: `CollisionObject`, `CollisionRequest`, `CollisionResult`, `DistanceRequest`, `DistanceResult`, `Contact`, `DifferentialContactData`, `collide`, `distance`, `collideBatch`.
- Backend: `BackendType`, `QueryMode`, `CpuBackend`, `GpuBackend`.
- I/O: `SDFBinReader`, `SDFBinWriter`, `ContactOnlySDFBin`.
- Adapters: dependency-free `FCLAdapter`, `EigenAdapterPlan`, `PythonBindingPlan`.

## 6. README Summary

The new README presents AdaSDF-CL as an adaptive compressed SDF collision library that complements BVH/FCL methods. It includes installation notes, minimal C++ usage, adaptive SDF build flow, `.sdfbin` load/query flow, GPU batch-query sketch, Python binding plan, file-format notes, application areas, roadmap, citation placeholder, license, and contributing guidance.

## 7. FCL-style Correspondence

- FCL-like geometry instance -> `adasdf::CollisionObject`
- FCL collision request -> `adasdf::CollisionRequest`
- FCL collision result -> `adasdf::CollisionResult`
- FCL distance request/result -> `adasdf::DistanceRequest` / `adasdf::DistanceResult`
- Optional adapter surface -> `adasdf::FCLStyleCollisionObject` and `adasdf::FCLAdapter`

The adapter does not include real FCL headers in v0.1.

## 8. Contact-only SDFBin Summary

Contact-only `.sdfbin` is documented as a compact query asset with magic number, version, unit, global bounding box, block metadata, local ranges, voxel size, resolution, compression rank, compressed SDF payload, near-surface error, reconstruction error, checksum, and query acceleration metadata. It explicitly excludes UI slice caches, debug images, temporary benchmark data, redundant intermediate matrices, and non-query information.

## 9. TODO

- Connect `adasdf::AdaptiveSDFBuilder` to `sdf::StlReader`, `sdf::OctreeSDF`, and `sdf::AdaptiveBlockCompressor`.
- Connect `SDFBinReader` and `SDFBinWriter` to `sdf::ModelIO`.
- Translate `sdf::SDFModelPackage` into `adasdf::SDFModel`.
- Implement CPU signed distance and gradient queries.
- Route `collide()` to `sdf::PairCollisionSDF` and all-points query paths.
- Add real `distance()` implementation.
- Define exact full `.sdfbin` and contact-only `.sdfbin` binary compatibility rules.
- Connect CUDA batch query only behind optional CUDA configuration.
- Add real FCL adapter target only as an optional integration.
- Add differentiable contact Jacobian support without claiming unsupported autodiff.

## 10. Next Round

1. Add `.gitignore` and choose which root artifacts belong in source control.
2. Implement `SDFBinReader::read` by wrapping `sdf::ModelIO::loadBinary`.
3. Implement `SDFModel::signedDistance` and `SDFModel::gradient` using loaded adaptive low-rank data.
4. Implement CPU `collide()` for `QueryMode::Balanced`.
5. Add compile/link tests for the first real CPU query path.
6. Add a converter or writer for contact-only `.sdfbin`.
7. Decide whether to move the public skeleton from `adasdf_cl/` to the repository root after the legacy workspace is cleaned.

## 11. Verification

- Configured the isolated interface preview with:
  `cmake -S adasdf_cl -B build/adasdf_cl-interface -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON`
- Built the interface test targets with MSVC through:
  `cmake --build build/adasdf_cl-interface --config Debug`
- Ran:
  `ctest --test-dir build/adasdf_cl-interface -C Debug --output-on-failure`
- Result: 4/4 interface tests passed.
