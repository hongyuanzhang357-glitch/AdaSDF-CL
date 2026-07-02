# Runtime Memory Strategy

AdaSDF-CL now exposes three runtime memory patterns for query workflows.

## Direct Compressed Query

Compressed adaptive block SDFs can be queried directly on CPU. Matrix-SVD grid
values are reconstructed on demand. This is useful for sparse queries, small
point sets, debugging, and fallback.

## Global Expansion

Global expansion creates a dense expanded layout for the full model. It is
simple and useful for benchmarking or GPU expanded-layout experiments, but it
can lose the memory advantage of compression.

## Active Block Cache

The v1.10.0-alpha active block cache expands only local blocks selected from
contact/query samples. It preserves the compressed model as the long-lived
asset and keeps only a small dense working set resident on CPU.

This is the recommended compressed-SDF runtime memory direction for sparse
contact workflows until CUDA active block cache and GPU-native compressed query
are implemented.
