# Build Profiler

Release: `v1.17.1-alpha`

The build profiler records coarse build-stage timings and counters without
changing SDF construction algorithms. It is off by default.

## CLI Flags

- `--profile`
- `--profile-json path`
- `--progress`
- `--progress-json path`
- `--max-seconds value`
- `--distance-backend brute_force|brute|bvh`
- `--threads auto|N`

`adasdf_build_dense_sdf`, `adasdf_build_adaptive_sdf`,
`adasdf_compress_adaptive_sdf`, and `adasdf_build_compressed_sdf` write
`adasdf.build_profile.v1` JSON when `--profile-json` is provided. `--profile`
writes the same JSON to stderr so it does not pollute normal JSON stdout
contracts.

## Timings

The profile includes `load_mesh_time_ms`, `mesh_preprocess_time_ms`,
`bvh_build_time_ms`, `distance_query_time_ms`, `sign_query_time_ms`,
`adaptive_refinement_time_ms`, `contact_band_marker_time_ms`,
`compression_time_ms`, `write_model_time_ms`, `metadata_time_ms`, and
`total_time_ms`.

## Counters

The profile includes mesh counts, grid/query counts, BVH visit/test counts,
contact-band counts, compression counts, fallback counts, and output bytes.

The profiler is a reporting layer. It does not imply a new acceleration claim.
