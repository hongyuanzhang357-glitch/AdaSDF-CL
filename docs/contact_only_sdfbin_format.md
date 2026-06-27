# Contact-only SDFBin Format

Contact-only `.sdfbin` is the minimal deployment format for engineering contact queries. It should contain only the data needed to evaluate signed distance, gradient/contact normals, and block lookup for collision or distance queries.

## Goals

- Smaller than full `.sdfbin`.
- Stable enough for engineering software integration.
- Free of UI caches, screenshots, temporary CSVs, and benchmark artifacts.
- Suitable for CPU and optional CUDA query backends.

## Required Data

- magic number
- version
- model unit
- global bounding box
- block count
- block metadata
- block origin
- block voxel size
- block resolution
- compression rank
- compressed SDF data
- near-surface error
- max reconstruction error
- optional checksum
- query acceleration metadata

## Excluded Data

- UI slice cache
- visualization debug images
- temporary benchmark data
- redundant intermediate matrices
- information not required for contact queries

## Suggested Binary Layout

```text
ContactOnlyHeader
BlockDirectory[block_count]
QueryAccelerationMetadata
CompressedPayloadTable
CompressedPayloadBytes
OptionalChecksum
```

## Header Fields

```text
magic: uint32, "ADCL"
version: uint32
unit: fixed or string table index
global_aabb_min: float64[3]
global_aabb_max: float64[3]
block_count: uint64
near_surface_error: float64
max_reconstruction_error: float64
flags: uint64
```

## Block Directory Fields

```text
block_id: int32
origin: float64[3]
local_min: float64[3]
local_max: float64[3]
voxel_size: float64
resolution: int32[3]
codec: uint32
rank: int32[3]
near_surface_error: float64
max_reconstruction_error: float64
payload_offset: uint64
payload_size: uint64
```

## Query Acceleration Metadata

The format should include enough data to map a local query point to one or more candidate blocks. The current implementation can likely derive this from `sdf::AdaptiveLowRankModel` micro-block maps or a compact block directory acceleration table.

## TODO

- Define exact endian behavior.
- Define checksum algorithm.
- Decide whether gradients are reconstructed from SDF samples or stored as optional compressed gradient blocks.
- Implement converter from full `.sdfbin` to contact-only `.sdfbin`.

## v0.4+ Status

The full `.sdfbin` writer path is now connected for existing-core-backed models, but contact-only export is still intentionally deferred. `BuildOptions::enable_contact_only_export` reserves the public option so callers can express the desired output mode once the converter and reader contract are implemented.
