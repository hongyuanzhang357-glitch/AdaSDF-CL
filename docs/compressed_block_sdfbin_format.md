# Compressed Block SDFBin Format

v1.7.0-alpha adds the audit-friendly text format:

```text
ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
```

The format stores a mixed set of compressed and dense-fallback adaptive blocks.
It is intended for core-free alpha workflows and human-readable validation.

## Top-Level Fields

Typical top-level fields include:

```text
unit m
signed 1
global_min x y z
global_max x y z
block_count B
compression_method mixed_matrix_svd_dense_fallback
original_memory_bytes ...
compressed_memory_bytes ...
compression_ratio ...
```

## Matrix-SVD Block

```text
block
id 0
source_block_id 0
node_id 12
level 3
near_surface 1
method MatrixSVD
min x y z
max x y z
origin x y z
spacing dx dy dz
resolution nx ny nz
rank r
error max mean rms p95
U
...
S
...
Vt
...
end_block
```

## Dense Fallback Block

```text
block
id 1
method DenseFallback
...
values
...
end_block
```

## Tool Support

The format is supported by:

- `SDFBinReader::read()`;
- `SDFBinWriter::write()`;
- `adasdf_info`;
- `adasdf_query`;
- `adasdf_collide`;
- `adasdf_expansion_quality`;
- `adasdf_benchmark_batch_query --model`.

GPU-native compressed query is not implemented in v1.7. CUDA benchmarks use the
same expanded SDF path used by other queryable public models.
