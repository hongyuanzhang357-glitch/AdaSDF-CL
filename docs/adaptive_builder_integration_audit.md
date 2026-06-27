# Adaptive Builder Integration Audit

This audit summarizes the AdaSDF-CL v0.4+ bridge from public builder APIs to the existing OctreeBlockLowRankSDF implementation.

## Implemented Path

```text
AdaptiveSDFBuilder::fromMesh(stl_path, options)
  -> ExistingBuilderBridge::buildFromMesh(...)
  -> sdf::readStl(...)
  -> sdf::OctreeSDF::build(...)
  -> sdf::AdaptiveBlockCompressor::build(...)
  -> sdf::makeModelMetadata(...)
  -> sdf::ModelIO::saveBinary(...)
  -> ExistingSDFBridge::loadExistingSDFBin(...)
```

`SDFBinWriter::write(output_path, model)` then uses the native model handle when available:

```text
SDFModel::nativeHandle()
  -> canWriteSDFBin()
  -> writeSDFBin(output_path)
  -> SDFBinReader::read(output_path)
  -> isValid()
```

## Public Surface

- `BuildOptions` now contains accuracy, memory, adaptive compression, DASH-SDF terminology, and compact existing-core bridge parameters.
- `AdaptiveSDFBuilder::fromMesh(path, options)` supports STL input.
- `AdaptiveSDFBuilder::fromSTL(path, options)` delegates to `fromMesh`.
- `AdaptiveSDFBuilder::fromOBJ(path, options)` is reserved and reports unsupported input.
- `AdaptiveSDFBuilder::fromMesh(const MeshModel&, options)` is reserved and reports unsupported input.
- `ExistingBuilderBridge` hides all `sdf::` types from public headers.
- `SDFBinWriter` writes only models whose native handle can serialize an existing-core package.
- `adasdf_build` exposes a command-line build path and reload validation.

## Option Mapping

| AdaSDF-CL option | Existing-core target |
| --- | --- |
| `max_octree_level` | `sdf::OctreeSDFOptions::maxLevel` |
| `box_margin_rate` | `sdf::OctreeSDFOptions::boxMarginRate` |
| `surface_band_cells` | `sdf::OctreeSDFOptions::surfaceBandCells` |
| `bvh_leaf_size` | `sdf::OctreeSDFOptions::bvhLeafSize` |
| `sign_ray_count` | `sdf::OctreeSDFOptions::signRayCount` |
| `base_block_cells` | `sdf::AdaptiveCompressionOptions::baseBlockCells` |
| `min_block_cells` | `sdf::AdaptiveCompressionOptions::minBlockCells` |
| `ghost_cells` | `sdf::AdaptiveCompressionOptions::ghostCells` |
| `min_rank` / `near_min_rank` / `max_rank` | adaptive rank controls |
| `near_surface_error` | converted to existing error-over-spacing policy |
| `compression_method` | Matrix SVD, Tucker, or Tensor Train selector |

The `near_surface_error` mapping is conservative but not exact. The existing core currently exposes `nearErrTolOverH`, so the bridge divides by the fine spacing and applies a practical lower bound.

## Validation Performed

- Configured with examples, tests, existing-core bridge, adaptive builder, and surrogate recommender enabled.
- Built Debug targets including `adasdf_build`, `adasdf_build_then_query`, and all tests.
- Ran 13 CTest tests successfully.
- Built a project-generated closed cube STL into `.sdfbin` using `adasdf_build`.
- Reloaded the generated `.sdfbin` with `SDFBinReader::read()`.
- Queried sample distances and gradients through the existing low-rank model.
- Ran the two-object collision example against the generated `.sdfbin`.
- Ran repository hygiene checks for local paths, large artifacts, generated arrays, and raw dataset directories.

## Known Boundaries

- STL input is the only real builder input in v0.4+.
- OBJ and in-memory mesh build routes are reserved.
- CUDA construction is unsupported.
- Memory limits are stored and surfaced, but the old compression pipeline is not yet a strict global memory-budget optimizer.
- Contact-only export is reserved by option and documentation only.
- DASH-SDF recommendation is a placeholder and never reports `credible=true`.

## Release Hygiene

- No external surrogate checkpoint was copied.
- No raw STL dataset was copied.
- No generated experiment directory was copied.
- No large build output is tracked under `adasdf_cl`.
- Existing MIT licensing was preserved for this subtree; license changes should be handled as a separate project decision.
