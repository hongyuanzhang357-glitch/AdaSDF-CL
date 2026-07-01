# Adaptive Block SDFBin Format

v1.6.0-alpha adds a text, audit-friendly adaptive block format:

```text
ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
unit m
signed 1
global_min x y z
global_max x y z
block_count B
block_resolution N
block
id 0
node_id 0
level 2
near_surface 1
min x y z
max x y z
origin x y z
spacing dx dy dz
resolution nx ny nz
values
...
end_block
```

The payload stores dense double-precision phi values per adaptive block. It
does not store low-rank factors, SVD matrices, Tucker cores, or surrogate
metadata.

`SDFBinReader::read()` detects this magic before the older demo/dense/existing
format paths. `SDFBinWriter::write()` can write `AdaptiveBlockSDFModel`
instances and validates them by reloading.

`adasdf_info` reports block count, near-surface block count, max level used,
block resolution, signed/unsigned status, and memory footprint.
