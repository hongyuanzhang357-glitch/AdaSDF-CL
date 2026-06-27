# SDFBin Format

The existing project already has binary `.sdfbin` model save/load support through `sdf::ModelIO` and `sdf::SDFModelPackage`. AdaSDF-CL should preserve backward compatibility while documenting a stable public contract.

## Purpose

Full `.sdfbin` stores an adaptive compressed SDF model plus metadata useful for reconstruction, validation, summary reports, and later conversion to smaller deployment formats.

## Recommended Sections

1. Header
   - magic number
   - format version
   - endian marker
   - scalar type
   - checksum mode

2. Model metadata
   - model name
   - source mesh path or source identifier
   - unit
   - global bounding box
   - fine-grid cell and node counts
   - fine voxel size
   - octree/build options

3. Block metadata
   - block ID
   - parent block ID when available
   - block depth
   - local cell/node ranges
   - origin and local coordinate range
   - voxel size and resolution
   - near-surface and sign-change flags

4. Compression metadata
   - compression codec
   - rank or tensor ranks
   - dense memory estimate
   - compressed memory estimate
   - retained energy when available
   - near-surface error
   - maximum reconstruction error
   - sign mismatch count

5. Compressed payload
   - Matrix SVD factors, Tucker factors, TT factors, or future codecs
   - per-block offset table
   - optional alignment padding

6. Query metadata
   - acceleration metadata needed to find blocks from local points
   - micro-block map or equivalent block lookup data
   - supported query modes

7. Diagnostics
   - optional build time, compression time, benchmark summaries, and validation statistics

## Compatibility Rules

- Readers should reject unknown required sections with a clear error.
- Readers may ignore unknown optional sections.
- Writers should store a version number and enough metadata to detect unit and coordinate mismatch.
- UI-only data should not be required to perform collision queries.

## Connection to Existing Code

AdaSDF-CL v0.4+ wraps:

- `sdf::ModelIO::inspectBinaryHeader`
- `sdf::ModelIO::loadBinary`
- `sdf::ModelIO::saveBinary`
- `sdf::SDFModelPackage`
- `sdf::AdaptiveLowRankModel`
- `sdf::OctreeSDF`
- `sdf::AdaptiveBlockCompressor`

The public reader path is `adasdf::SDFBinReader::read()`. The loaded `SDFModel` exposes metadata, AABB, approximate memory footprint, near-surface error, `sampleDistance()`, and finite-difference `sampleGradient()`.

The public builder/writer path is `adasdf::AdaptiveSDFBuilder::fromMesh(stl_path, options)` followed by `adasdf::SDFBinWriter::write(output_path, model)`. Writer support currently requires a model whose native handle can write the existing-core package.

## v0.4+ Limitations

- The binary format is not changed.
- Only existing `SDFLRM01` binary files readable by `sdf::ModelIO` are supported.
- Query coordinates are interpreted in the model/root coordinate system stored in the file.
- Gradient is central finite difference over the existing signed-distance query.
- New `.sdfbin` construction is bridged for STL only.
- `SDFBinWriter` is not a generic serializer for arbitrary future `SDFModel` implementations.
- Contact-only `.sdfbin` remains a design document, not a writer/reader implementation.
