# CUDA Active Block Cache

AdaSDF-CL v1.11.0-alpha adds an optional CUDA active block cache path for
sparse/contact workflows.

The pipeline is deliberately bounded:

1. CPU selects active block ids from sparse samples.
2. CPU expands those active blocks into local dense grids.
3. The expanded active dense blocks are uploaded to CUDA.
4. CUDA performs local block lookup and trilinear dense-grid interpolation.
5. Optional normals are computed by finite differences inside the selected
   local block.

This is not GPU-native compressed SVD reconstruction. It does not reconstruct
low-rank factors per point on GPU. It is a bridge between the v1.10 CPU active
block cache and a CUDA local query kernel.

## CLI

```bash
adasdf_cuda_active_block_query model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --phi-only \
  --out cuda_active_query.csv \
  --report cuda_active_query.md
```

Use `--with-normal` for phi+normal output.

Return codes:

- `0`: query completed and no collision status was reported.
- `10`: query completed and collision was detected.
- `20`: CUDA was unavailable and the tool skipped the CUDA path.

CPU-only builds still compile this tool. On machines without a CUDA runtime it
reports `CUDA available: false` and returns `20`.

## Current Limits

- Active block selection and expansion happen on CPU.
- Uploaded active blocks are reuploaded for each query in the v1.11 baseline.
- Block lookup in the CUDA kernel is a deterministic linear scan over uploaded
  active blocks.
- Fallback samples can be resolved with the CPU model query path.

The next optimization step is persistent GPU resident active-block reuse plus a
deterministic active block index map for faster local lookup.

## v1.17 Lookup Metadata

v1.17.0-alpha adds a CPU-built flat lookup metadata export on
`CudaActiveBlockCache`. The exported table contains keys, block ids, cache
slots, and bounds. It is intended for future GPU-side binary search or flat
lookup work.

This is not a CUDA hash table. Active block selection and expansion still occur
on CPU, and compressed SVD reconstruction is still not GPU-native.
