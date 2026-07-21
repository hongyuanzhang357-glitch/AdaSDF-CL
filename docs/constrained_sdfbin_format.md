# Constrained SDFBin Format V1

Magic: `ADASDF_C3D_V1`

Format version: `1`

The constrained format is separate from the legacy Dense, AdaptiveBlock, and
CompressedBlock formats. Existing readers and files remain unchanged.
`SDFBinReader` dispatches the new magic to `ConstrainedSDFModel`; unknown or
unsupported files continue to use the existing-core bridge and fail with an
explicit error when unsupported.

## Layout

1. Fixed magic, format version, endian marker, and total file byte count.
2. Octree node and block counts.
3. Global AABB and serialized-reload zero-surface error metadata.
4. Fixed-size octree records with deterministic IDs, parents, children,
   levels, bounds, leaf flags, near-surface flags, and sign-change flags.
5. Fixed-size block directory records with bounds, tensor dimensions,
   algorithm, ranks, tolerance, actual compression error, original/compressed
   bytes, decoded peak bytes, payload offset/length, and payload checksum.
6. Per-block payloads. Payload vectors are length-prefixed and interpreted by
   the directory algorithm (`Dense`, `MatrixSVD`, `HOSVD`, `TT`, or `Tucker`).
7. Whole-file FNV-1a 64-bit checksum footer.

The reader validates magic, version, endian marker, counts, every offset and
length, whole-file checksum, block checksum, tensor vector lengths, and method
range before making a model queryable. Truncated or corrupt files are rejected.

V1 uses native little-endian IEEE-754 doubles and an explicit endian marker.
The current reader rejects byte-swapped files instead of silently decoding
them. A future V2 may define canonical byte swapping.

## Runtime behavior

Opening verifies the compressed file and loads metadata/directories, not dense
block tensors. `ConstrainedSDFModel` locates one block for a point, loads and
caches that compressed payload, evaluates the eight neighboring tensor values
directly from compressed factors, and performs trilinear interpolation. It
does not globally expand the SDF.

